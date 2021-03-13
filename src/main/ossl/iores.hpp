#pragma once

#include <yasync/io.hpp>
#include "ssl.hpp"

namespace yasync::io::ssl {

/**
 * Opens an OpenSSL layer over the resource
 */
result<IOResource, std::string> openSSLIO(IOResource, const SSLContext&);

}
