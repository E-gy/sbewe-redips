#include "ssl.hpp"

// #include <openssl/applink.c>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace redips::ssl {

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