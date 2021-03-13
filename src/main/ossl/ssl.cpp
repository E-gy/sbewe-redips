#include "ssl.hpp"

// #include <openssl/applink.c>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <sstream>

namespace yasync::io::ssl {

std::string printSSLError(const std::string& message, sslerr_t e){
	std::ostringstream compose;
	compose << message << ": ";
	char buff[512] = {};
	ERR_error_string_n(e, buff, sizeof(buff)-1);
	compose << buff;
	return compose.str();
}
std::string printSSLError(const std::string& message){ return printSSLError(message, ERR_get_error()); }

SSLStateControl::SSLStateControl(){
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
}
SSLStateControl::~SSLStateControl(){
	ERR_free_strings();
	EVP_cleanup();
}

SSLContext::SSLContext(SSL_CTX* ctx) : context(ctx) {}
SSLContext::~SSLContext(){
	if(context) SSL_CTX_free(context);
}
SSLContext::SSLContext(SSLContext && o){
	context = o.context;
	o.context = nullptr;
}
SSLContext& SSLContext::operator=(SSLContext && o){
	if(context) SSL_CTX_free(context);
	context = o.context;
	o.context = nullptr;
}

result<SharedSSLContext, std::string> createSSLContext(const std::string& cert, const std::string& pk){
	SSL_CTX* ctx = SSL_CTX_new(TLS_method());
	if(!ctx) return retSSLError<result<SharedSSLContext, std::string>>("Failed to create TLS negotiations context");
	SharedSSLContext sctx(new SSLContext(ctx));
	if(SSL_CTX_use_certificate_chain_file(ctx, cert.c_str()) != 1) return retSSLError<result<SharedSSLContext, std::string>>("Failed to load certificate from file");
	if(SSL_CTX_use_PrivateKey_file(ctx, pk.c_str(), SSL_FILETYPE_PEM) != 1) return retSSLError<result<SharedSSLContext, std::string>>("Failed to load pk from file");
	if(SSL_CTX_check_private_key(ctx) != 1) return retSSLError<result<SharedSSLContext, std::string>>("PK invalid");
	return sctx;
}

}
