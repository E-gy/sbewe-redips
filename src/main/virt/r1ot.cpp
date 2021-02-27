#include "r1ot.hpp"

#include <iostream>

using namespace magikop;

namespace redips::virt {

class R1oter : public IServer {
	std::unordered_map<std::string, SServer> services;
	R1oter(std::unordered_map<std::string, SServer> && ss) : services(ss) {}
	public:
		friend class R1otBuilder;
		void take(yasync::io::IOResource conn, redips::http::SharedRequest req) override {
			if(auto host = req->getHeader(http::Header::Host)) if(services.count(*host) > 0) return services[*host]->take(conn, req);
			http::Response resp;
			resp.status = http::Status::NOT_FOUND;
			resp.setBody("No such service available");
			auto wr = conn->writer();
			resp.write(wr);
			conn->engine->engine <<= wr->eod() >> ([](){}|[](auto err){ std::cerr << "Error sending 'no host' response: " << err << "\n"; });
		}
};

R1otBuilder& R1otBuilder::addService(const std::string& n, SServer s){
	services[n] = s;
	return *this;
}
SServer R1otBuilder::build(){ return SServer(new R1oter(std::move(services))); }

}