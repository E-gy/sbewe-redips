#pragma once

#include <yasync.hpp>
#include <yasync/ticktack.hpp>
#include <util/ip.hpp>
#include <functional>

namespace redips::fiz {

using namespace magikop;

using ConnectionFactory = std::function<yasync::Future<yasync::io::ConnectionResult>()>;

result<ConnectionFactory, std::string> findConnectionFactory(const IPp& ipp, yasync::io::IOYengine* engine, yasync::TickTack* ticktack, const yasync::TickTack::Duration& timeout);

}
