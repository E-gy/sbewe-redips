#include "iores.hpp"

#include <array>
#include <iostream>
#include <openssl/err.h>
#include <thread>

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
	/// whether SNI has occured
	bool inictx = false;
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
		inline auto& hedlog(){ return std::cout << "[" << this << "] "; }
		result<void, std::string> initBIO(){
			if(!ssl) return retSSLError<result<void, std::string>>("SSL resource in errored state");
			if(!(bioIn = BIO_new(BIO_s_mem()))) return retSSLError<result<void, std::string>>("BIO in construction failed");
			if(!(bioOut = BIO_new(BIO_s_mem()))) return retSSLError<result<void, std::string>>("BIO in construction failed");
			SSL_set0_rbio(ssl, bioIn);
			SSL_set0_wbio(ssl, bioOut);
			return result<void, std::string>::Ok();
		}
		/**
		 * Whether, and how much, data SSL wants to send
		 */
		size_t sslWantsToSend(){
			return BIO_ctrl_pending(bioOut);
		}
		/**
		 * Whether, and how much, data SSL made ready for consumption
		 */
		size_t sslMadeDataReady(){
			return SSL_pending(ssl);
		}
		/**
		 * Yeets everything in out BIO into raw.
		 * Returns future that completes when everything got yeeted
		 */
		Future<WriteResult> yeetPending(){
			auto pending = sslWantsToSend();
			if(!pending){
				hedlog() << "nothing to yeet\n";
				return completed(WriteResult::Ok());
			}
			std::vector<char> wrout;
			wrout.resize(pending);
			auto mvd = BIO_read(bioOut, wrout.data(), pending);
			hedlog() << "querying yeet " << pending << " (" << mvd << ") bytes\n";
			if(pending == 7){
				for(unsigned i = 0; i < pending; i++) std::cout << (unsigned)wrout[i] << " ";
				std::cout << "\n";
			}
			return engine <<= raw->write(wrout);
		}
		/**
		 * When read query to raw completes, handles transferring the data through SSL
		 * @returns whether EOT is reached
		 */
		result<bool, std::string> handleReadCompleted(){
			if(!rip) return result<bool, std::string>::Err("Lol no");
			auto rres = *((*rip)->result());
			rip = std::nullopt;
			if(auto err = rres.err()) return result<bool, std::string>::Err(*err);
			//Transfer available data to SSL
			while(true){
				auto data = rres.ok();
				hedlog() << "received " << data->size() << " bytes\n";
				if(data->size() == 0) return result<bool, std::string>::Ok(true); 
				size_t wr = 0;
				while(wr < data->size()) wr += BIO_write(bioIn, data->data() + wr, data->size() - wr); //BIO to memory is sync duh, *and* always succeeds as long as there's memory available
				if(!inictx){ //SNI has yet to switch context. Retain initial hadshake data. Query read; resubmit
					inictx = true;
					auto r = SSL_read(ssl, buffer.data(), buffer.size());
					if(r <= 0){
						auto err = SSL_get_error(ssl, r);
						if(err == SSL_ERROR_WANT_READ){
							continue;
						} else hedlog() << printSSLError("Yeah, no ") << "\n";
					} else hedlog() << "ughm, wut?\n"; //well that is unexpected. uhh wtf?
				}
				break;
			}
			return result<bool, std::string>::Ok(false);
			//TryResendBufferedData idk wut???
		}
		/// Yeets pending data, puts the future in wip and returns it 
		inline auto justYeetAlready(){ return *(wip = yeetPending()); }
		/// Queries raw to read next chunk, puts the future in rip and returns it
		inline auto justReadAlready(){
			hedlog() << "querying read\n";
			return *(rip = engine <<= raw->_read(1));
		}
		Future<ReadResult> _read(size_t bytes) override {
			return defer(lambdagen([this, self = slf.lock(), bytes]([[maybe_unused]] const Yengine* _engine, bool& done, std::vector<char>& resd) -> std::variant<AFuture, movonly<ReadResult>> {
				if(done) return ReadResult::Ok(resd);
				hedlog() << "-- On " << std::this_thread::get_id() << "\n";
				if(rip){
					auto rres = handleReadCompleted();
					if(auto err = rres.err()){
						done = true;
						return ReadResult::Err(*err);
					}
					if(*rres.ok()){
						done = true;
						hedlog() << "reached EOF\n";
						return ReadResult::Ok(resd);
					}
					hedlog() << "read completed\n";
				}
				if(wip){
					auto rres = *((*wip)->result());
					wip = std::nullopt;
					if(auto err = rres.err()){
						done = true;
						return ReadResult::Err(*err);
					}
					hedlog() << "yeet completed\n";
				}
				hedlog() << "checking black box...\n";
				hedlog() << "Out pending: " << BIO_ctrl_pending(bioOut) << "\n";
				hedlog() << "In write guarantee: " << BIO_ctrl_get_write_guarantee(bioOut) << "\n";
				hedlog() << "In read request: " << BIO_ctrl_get_read_request(bioOut) << "\n";
				hedlog() << "Black box state: " << SSL_get_state(ssl) << "\n";
				hedlog() << "Black box is pre: " << SSL_in_before(ssl) << "\n";
				hedlog() << "Black box is init: " << SSL_in_init(ssl) << "\n";
				hedlog() << "Black box is post: " << SSL_is_init_finished(ssl) << "\n";
				if(SSL_in_before(ssl)) return justReadAlready(); //Trigger initial read
				if(sslWantsToSend()) return justYeetAlready();
				if(SSL_in_init(ssl)) return justReadAlready();
				if(SSL_is_init_finished(ssl)){
					hedlog() << "attempting SSL read\n";
					while(true){
						ERR_clear_error();
						auto r = SSL_read(ssl, buffer.data(), buffer.size());
						hedlog() << r << "\n";
						if(r <= 0){
							auto err = SSL_get_error(ssl, r);
							hedlog() << err << "\n";
							if(sslWantsToSend()) err = SSL_ERROR_WANT_WRITE;
							switch(err){
								case SSL_ERROR_WANT_WRITE: return justYeetAlready(); //SSL handshake in progress, wants to send data
								case SSL_ERROR_WANT_READ: return justReadAlready(); //SSL handshake in progress, needs more data
								case SSL_ERROR_SSL: //A non-recoverable, fatal error in the SSL library occurred, usually a protocol error. OpenSSL error queue contains more information on the error.
									done = true;
									return retSSLError<ReadResult>("Fatal SSL error :/");
								case SSL_ERROR_SYSCALL:{ //Some non-recoverable, fatal I/O error occurred. The OpenSSL error queue may contain more information on the error.
									done = true;
									auto nerr = ERR_get_error();
									return retSSLError<ReadResult>("Fatal SSL I/O error :/", nerr ? nerr : err); //may lol
								}
								default:
									done = true;
									return retSSLError<ReadResult>("SSL read failed", err);
							}
						}
						resd.insert(resd.end(), buffer.begin(), buffer.begin() + r);
						if(bytes > 0 && resd.size() > bytes){
							done = true;
							return ReadResult::Ok(resd);
						}
					}
				}
				//Uh oh, how did we get here?
				done = true;
				return ReadResult::Err("Reached undetermined read state");
			}, std::vector<char>()));
		}
		Future<WriteResult> _write(std::vector<char>&& data) override {
			return defer(lambdagen([this, self = slf.lock()]([[maybe_unused]] const Yengine* _engine, bool& done, std::vector<char>& wrb) -> std::variant<AFuture, movonly<WriteResult>> {
				if(done) return WriteResult::Ok();
				if(rip){
					auto rres = handleReadCompleted();
					if(auto err = rres.err()){
						done = true;
						return WriteResult::Err(*err);
					}
					if(*rres.ok()){
						done = true;
						hedlog() << "reached EOF\n";
						return WriteResult::Err("Reached EOT when querying handshake data");
					}
					hedlog() << "read completed\n";
				}
				if(wip){
					auto rres = *((*wip)->result());
					wip = std::nullopt;
					if(auto err = rres.err()){
						done = true;
						return WriteResult::Err(*err);
					}
					hedlog() << "yeet completed\n";
				}
				hedlog() << "checking black box...\n";
				if(sslWantsToSend()) return justYeetAlready();
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
								return retSSLError<WriteResult>("SSL write failed", err);
						}
					}
					wrb.erase(wrb.begin(), wrb.begin()+w);
					if(wrb.size() == 0) return justYeetAlready(); //Written out all user data; yeet!
				}
			}, data));
		}
		void notify([[maybe_unused]] IOCompletionInfo inf) override {} //No-Op because we're a proxy :D
};

result<IOResource, std::string> openSSLIO(IOResource raw, SSL* ssl){
	auto sslres = std::make_shared<SSLResource>(raw, ssl);
	auto bres = sslres->initBIO();
	if(auto err = bres.err()) return result<IOResource, std::string>::Err(*err);
	return result<IOResource, std::string>::Ok(sslres);
}
result<IOResource, std::string> openSSLIO(IOResource raw, const SSLContext& ctx){
	auto ssl = SSL_new(ctx.ctx());
	if(!ssl) return retSSLError<result<IOResource, std::string>>("SSL instantiation from context failed");
	return openSSLIO(raw, ssl);
}
result<IOResource, std::string> openSSLIO(IOResource raw, const SharedSSLContext& ctx){ return openSSLIO(raw, *ctx); }

}
