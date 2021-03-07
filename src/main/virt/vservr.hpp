#pragma once

#include <http/rr.hpp>
#include <functional>

namespace redips::virt {

class IServer {
	public:
		using RespBack = std::function<void(redips::http::Response&&)>;
		virtual void take(yasync::io::IOResource, redips::http::SharedRequest, RespBack) = 0;
};

using SServer = std::shared_ptr<IServer>;

}
