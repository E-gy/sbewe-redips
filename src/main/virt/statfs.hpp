#pragma once

#include "vservr.hpp"

#include <string>

namespace redips::virt {

class StaticFileServer : public IServer {
	std::string root;
	std::string deff;
	public:
		StaticFileServer(std::string root, std::string deff);
		void take(yasync::io::IOResource, redips::http::SharedRequest, RespBack) override;
};

}
