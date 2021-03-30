#pragma once

#include "vservr.hpp"

#include <string>

namespace redips::virt {

class StaticFileServer : public IServer {
	yasync::io::IOYengine* const engine;
	std::string root;
	std::string deff;
	bool gmd;
	public:
		StaticFileServer(yasync::io::IOYengine*, std::string root, std::string deff, bool gmd);
		void take(const ConnectionInfo&, redips::http::SharedRRaw, RespBack) override;
};

}
