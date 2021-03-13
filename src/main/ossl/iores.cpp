#include "iores.hpp"

namespace yasync::io::ssl {

class SSLResource : public IAIOResource {
	IOResource raw;
	SSL* ssl = nullptr;
	BIO* bioIn = nullptr;
	BIO* bioOut = nullptr;
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
};

result<IOResource, std::string> openSSLIO(IOResource raw, const SSLContext& ctx){
	auto ssl = SSL_new(ctx.ctx());
	if(!ssl) return retSSLError<result<IOResource, std::string>>("SSL instantiation from context failed");
	auto sslres = std::make_shared<SSLResource>(raw, ssl);
	if(auto err = sslres->initBIO().err()) return result<IOResource, std::string>::Err(*err);
	return result<IOResource, std::string>::Ok(sslres);
}

}
