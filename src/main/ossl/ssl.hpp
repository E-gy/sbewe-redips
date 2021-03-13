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

}
