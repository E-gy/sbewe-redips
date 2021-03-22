#pragma once

#include <http/rr.hpp>
#include <functional>

namespace redips::virt {

class IServer {
	public:
		using RespBack = std::function<void(redips::http::RRaw&&)>;
		virtual void take(yasync::io::IOResource, redips::http::SharedRRaw, RespBack) = 0;
};

using SServer = std::shared_ptr<IServer>;

}
