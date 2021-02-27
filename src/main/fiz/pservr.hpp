#pragma once

#include <yasync/io.hpp>
#include <virt/vservr.hpp>

namespace redips::fiz {

class IListener {
	public:
		virtual ~IListener() = default;
		virtual yasync::Future<void> shutdown() = 0;
};

using SListener = std::shared_ptr<IListener>;

result<SListener, std::string> listenOn(yasync::io::IOYengine*, const std::string& addr, unsigned port, virt::SServer);

}
