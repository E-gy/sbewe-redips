#pragma once

#include <yasync.hpp>
#include <yasync/ticktack.hpp>
#include <util/ip.hpp>
#include <functional>
#include <utility>

namespace redips::fiz {

using namespace magikop;

using ConnectionFactoryResult = result<std::pair<yasync::io::IOResource, IPp>, std::string>;
using ConnectionFactory = std::function<yasync::Future<ConnectionFactoryResult>()>;

result<ConnectionFactory, std::string> findConnectionFactory(const IPp& ipp, yasync::io::IOYengine* engine, yasync::TickTack* ticktack, const yasync::TickTack::Duration& timeout, unsigned shora = 3);

}
