#pragma once

#include <http/rr.hpp>

namespace redips::virt {

class IServer {
	public:
		virtual void take(yasync::io::IOResource, redips::http::SharedRequest) = 0;
};

using SServer = std::shared_ptr<IServer>;

}
