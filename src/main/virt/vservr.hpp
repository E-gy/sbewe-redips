#pragma once

#include <http/rr.hpp>

namespace redips::virt {

class IServer {
	public:
		virtual void take(redips::http::SharedRequest) = 0;
};

}
