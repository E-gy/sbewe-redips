#include "r1ot.hpp"

#include <iostream>

namespace redips::virt {

using namespace magikop;

class R1oter : public IServer {
	std::unordered_map<std::string, SServer> services;
	R1oter(std::unordered_map<std::string, SServer> && ss) : services(ss) {}
	public:
		friend class R1otBuilder;
		void take(yasync::io::IOResource conn, redips::http::SharedRequest req, RespBack respb) override {
			if(auto host = req->getHeader(http::Header::Host)) if(services.count(*host) > 0) return services[*host]->take(conn, req, respb);
			respb(http::Response(http::Status::NOT_FOUND, "No such service available"));
		}
};

R1otBuilder& R1otBuilder::addService(const std::string& n, SServer s){
	services[n] = s;
	return *this;
}
SServer R1otBuilder::build(){ return SServer(new R1oter(std::move(services))); }

}