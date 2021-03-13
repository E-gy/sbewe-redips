#include "iores.hpp"

#include <array>

namespace yasync::io::ssl {

using namespace magikop;

/// https://stackoverflow.com/a/4426234
class SSLResource : public IAIOResource {
	IOResource raw;
	SSL* ssl = nullptr;
	/// When we read data from raw, we dump it here to make it accessible for subsequent SSL read
	BIO* bioIn = nullptr;
	/// When we need to write data, we SSL write it then yeet everything from here into raw
	BIO* bioOut = nullptr;
	/// Current read operation in progress
	std::optional<Future<ReadResult>> rip = std::nullopt;
	/// Current write operation in progress
	std::optional<Future<WriteResult>> wip = std::nullopt;
	std::array<char, 4096> buffer;
	public:
		SSLResource(IOResource r, SSL* s) : IAIOResource(r->engine), raw(r), ssl(s) {}
		~SSLResource(){
			if(ssl){ //SSL_set0_*bio transfers ownership and the SSL object frees them using BIO_free_all on free
				SSL_shutdown(ssl);
				SSL_free(ssl);
			}
		}
		result<void, std::string> initBIO(){
			if(!ssl) return retSSLError<result<void, std::string>>("SSL resource in errored state");
			if(!(bioIn = BIO_new(BIO_s_mem()))) return retSSLError<result<void, std::string>>("BIO in construction failed");
			if(!(bioOut = BIO_new(BIO_s_mem()))) return retSSLError<result<void, std::string>>("BIO in construction failed");
			SSL_set0_rbio(ssl, bioIn);
			SSL_set0_wbio(ssl, bioOut);
			return result<void, std::string>::Ok();
		}
		/**
		 * Yeets everything in out BIO into raw.
		 * Returns future that completes when everything got yeeted
		 */
		Future<WriteResult> yeetPending(){
			auto pending = BIO_ctrl_pending(bioOut);
			if(!pending) return completed(WriteResult::Ok());
			std::vector<char> wrout;
			wrout.reserve(pending);
			BIO_read(bioOut, wrout.data(), pending);
			return engine->engine <<= raw->write(wrout);
		}
		/**
		 * When read query to raw completes, handles transferring the data through SSL
		 */
		template<typename R> std::variant<AFuture, movonly<R>> handleReadCompleted(bool& done){
			if(!rip) return R::Err("Lol no");
			auto rres = *((*rip)->result());
			rip = std::nullopt;
			if(auto err = rres.err()){
				done = true;
				return R::Err(*err);
			}
			//Transfer available data to SSL
			auto data = rres.ok();
			size_t wr = 0;
			while(wr < data->size()) wr += BIO_write(bioIn, data->data() + wr, data->size() - wr); //BIO to memory is sync duh, *and* always succeeds as long as there's memory available
			return justYeetAlready(); 
			//TryResendBufferedData idk wut???
		}
		/// Yeets pending data, puts the future in wip and returns it 
		inline auto justYeetAlready(){ return *(wip = yeetPending()); }
		/// Queries raw to read next chunk, puts the future in rip and returns it
		inline auto justReadAlready(){ return *(rip = engine->engine <<= raw->_read(1)); }
		Future<ReadResult> _read(size_t bytes) override {
			return defer(lambdagen([this, self = slf.lock(), bytes]([[maybe_unused]] const Yengine* _engine, bool& done, std::vector<char>& resd) -> std::variant<AFuture, movonly<ReadResult>> {
				if(done) return ReadResult::Ok(resd);
				if(rip) return handleReadCompleted<ReadResult>(done);
				if(wip){
					auto rres = *((*wip)->result());
					wip = std::nullopt;
					if(auto err = rres.err()){
						done = true;
						return ReadResult::Err(*err);
					}
					//Each BIO write is followed by SSL read (eventually)
					while(true){
						auto r = SSL_read(ssl, buffer.data(), buffer.size());
						if(r <= 0){
							auto err = SSL_get_error(ssl, r);
							switch(err){
								case SSL_ERROR_WANT_WRITE: return justYeetAlready(); //SSL handshake in progress, wants to send data
								case SSL_ERROR_WANT_READ: return justReadAlready(); //SSL handshake in progress, needs more data
								default:
									done = true;
									return retSSLError<ReadResult>("SSL read failed", err);
							}
						}
						resd.insert(resd.end(), buffer.begin(), buffer.begin() + r);
						if(bytes > 0 && resd.size() > bytes){ //FIXME What if bytes is 0? How does SSL detect EOT?
							done = true;
							return ReadResult::Ok(resd);
						}
					}
				}
				//Let's start by reading som bytes
				return justReadAlready();
			}, std::vector<char>()));
		}
		Future<WriteResult> _write(std::vector<char>&& data) override {
			return defer(lambdagen([this, self = slf.lock()]([[maybe_unused]] const Yengine* _engine, bool& done, std::vector<char>& wrb) -> std::variant<AFuture, movonly<WriteResult>> {
				if(done) return WriteResult::Ok();
				if(rip) return handleReadCompleted<WriteResult>(done);
				if(wip){
					auto rres = *((*wip)->result());
					wip = std::nullopt;
					if(rres.isErr()){
						done = true;
						return rres;
					}
					if(wrb.size() == 0){ //Last write successfully flushed everything
						done = true;
						return WriteResult::Ok();
					}
				}
				//Let's yeet some data
				while(true){
					auto w = SSL_write(ssl, wrb.data(), wrb.size());
					if(w <= 0){
						auto err = SSL_get_error(ssl, w);
						switch(err){
							case SSL_ERROR_WANT_WRITE: return justYeetAlready(); //SSL handshake in progress, wants to send data
							case SSL_ERROR_WANT_READ: return justReadAlready(); //SSL handshake in progress, needs more data 
							default:
								done = true;
								return retSSLError<WriteResult>("SSL read failed", err);
						}
					}
					wrb.erase(wrb.begin(), wrb.begin()+w);
					if(wrb.size() == 0) return justYeetAlready(); //Written out all user data; yeet!
				}
			}, data));
		}
		void notify([[maybe_unused]] IOCompletionInfo inf) override {} //No-Op because we're a proxy :D
};

result<IOResource, std::string> openSSLIO(IOResource raw, const SSLContext& ctx){
	auto ssl = SSL_new(ctx.ctx());
	if(!ssl) return retSSLError<result<IOResource, std::string>>("SSL instantiation from context failed");
	auto sslres = std::make_shared<SSLResource>(raw, ssl);
	if(auto err = sslres->initBIO().err()) return result<IOResource, std::string>::Err(*err);
	return result<IOResource, std::string>::Ok(sslres);
}

}
