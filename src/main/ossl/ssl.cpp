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

}