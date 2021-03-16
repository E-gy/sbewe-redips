#pragma once

#include "vservr.hpp"

#include <string>

namespace redips::virt {

class StaticFileServer : public IServer {
	yasync::io::IOYengine* const engine;
	std::string root;
	std::string deff;
	public:
		StaticFileServer(yasync::io::IOYengine*, std::string root, std::string deff);
		void take(yasync::io::IOResource, redips::http::SharedRequest, RespBack) override;
};

}
