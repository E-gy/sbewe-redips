#pragma once

#include <yasync/io.hpp>
#include "ssl.hpp"

namespace yasync::io::ssl {

/**
 * Opens an OpenSSL layer over the resource on given SSL.
 * Note: ATM SSL IO does not support EOT/EOF detection, and thus either the size or delimiter must be known
 * @consumes SSL
 */
result<IOResource, std::string> openSSLIO(IOResource, SSL*);
/**
 * Opens an OpenSSL layer over the resource on given context
 */
result<IOResource, std::string> openSSLIO(IOResource, const SSLContext&);

}
