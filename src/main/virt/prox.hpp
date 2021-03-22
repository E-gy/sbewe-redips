#pragma once

#include <yasync.hpp>
#include "vservr.hpp"
#include <functional>
#include <iostream>

namespace redips::virt {

using ProxyConnectionFactory = std::function<yasync::Future<yasync::io::ConnectionResult>()>;

SServer proxyTo(yasync::Yengine*, ProxyConnectionFactory &&, unsigned retries = 0);

}
