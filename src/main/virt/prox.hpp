#pragma once

#include <yasync.hpp>
#include "vservr.hpp"
#include <functional>
#include <iostream>
#include <util/ip.hpp>

namespace redips::virt {

using ProxyConnectionFactoryResult = result<std::pair<yasync::io::IOResource, IPp>, std::string>;
using ProxyConnectionFactory = std::function<yasync::Future<ProxyConnectionFactoryResult>()>;
using ProxyConnectionUnfactory = std::function<void(yasync::io::ConnectionResult)>;

/// Returning this 
constexpr auto CONFAERREVERY1ISDED = "<{([everyone is dead bro])}>";

SServer proxyTo(yasync::Yengine*, ProxyConnectionFactory &&, unsigned retries = 0);
SServer proxyTo(yasync::Yengine*, ProxyConnectionFactory &&, ProxyConnectionUnfactory &&, unsigned retries = 0);

}
