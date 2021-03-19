#pragma once

#include <yasync/io.hpp>
#include <util/ip.hpp>
#include <virt/vservr.hpp>
#include "preqh.hpp"

namespace redips::fiz {

class IListener {
	public:
		virtual ~IListener() = default;
		virtual yasync::Future<void> onShutdown() = 0;
		virtual void shutdown() = 0;
};

using SListener = std::shared_ptr<IListener>;

result<SListener, std::string> listenOn(yasync::io::IOYengine*, const IPp&, ConnectionCare*, virt::SServer);

}
