#pragma once

#include <yasync/io.hpp>
#include <util/ip.hpp>
#include <virt/vservr.hpp>

namespace redips::fiz {

class IListener {
	public:
		virtual ~IListener() = default;
		virtual yasync::Future<void> onShutdown() = 0;
		virtual void shutdown() = 0;
};

using SListener = std::shared_ptr<IListener>;

result<SListener, std::string> listenOn(yasync::io::IOYengine*, const IPp& ipp, virt::SServer);

}
