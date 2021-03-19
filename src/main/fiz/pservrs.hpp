#pragma once

#include "pservr.hpp"
#include <util/hostresolv.hpp>
#include <ossl/ssl.hpp>

namespace redips::fiz {

result<SListener, std::string> listenOnSecure(yasync::io::IOYengine*, const IPp&, const HostMapper<yasync::io::ssl::SharedSSLContext>& hmap, ConnectionCare*, virt::SServer);

}
