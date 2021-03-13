#pragma once

#include <memory>
#include <string>
#include <util/result.hpp>
#include <openssl/ssl.h>

namespace yasync::io::ssl {

using sslerr_t = unsigned long;
std::string printSSLError(const std::string& message, sslerr_t e);
std::string printSSLError(const std::string& message);
template<typename R> R retSSLError(const std::string& message, sslerr_t e){ return R::Err(printSSLError(message, e)); }
template<typename R> R retSSLError(const std::string& message){ return R::Err(printSSLError(message)); }

class SSLStateControl {
	public:
		SSLStateControl();
		~SSLStateControl();
		SSLStateControl(const SSLStateControl&) = delete;
		SSLStateControl(SSLStateControl&&) = delete;
};

class SSLContext {
	SSL_CTX* context;
	public:
		SSLContext(SSL_CTX* ctx);
		~SSLContext();
		SSLContext(SSLContext &&);
		SSLContext& operator=(SSLContext &&);
		SSLContext(const SSLContext&) = delete;
		SSLContext& operator=(const SSLContext&) = delete;
		inline SSL_CTX* ctx() const { return context; }
};

using SharedSSLContext = std::shared_ptr<SSLContext>;

/**
 * Creates plain SSL context
 */
result<SharedSSLContext, std::string> createSSLContext();
/**
 * Creates SSL context with specified certificate and pk
 */
result<SharedSSLContext, std::string> createSSLContext(const std::string& cert, const std::string& pk);

}
