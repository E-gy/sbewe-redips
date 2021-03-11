#include "r1ot.hpp"

#include <iostream>

namespace redips::virt {

using namespace magikop;

class R1oter : public IServer {
	std::unordered_map<std::string, SServer> services;
	std::unordered_map<std::string, SServer> services2;
	std::optional<SServer> defolt;
	R1oter(std::unordered_map<std::string, SServer> && ss, std::unordered_map<std::string, SServer> && ss2, std::optional<SServer> && d) : services(ss), services2(ss2), defolt(d) {}
	public:
		friend class R1otBuilder;
		void take(yasync::io::IOResource conn, redips::http::SharedRequest req, RespBack respb) override {
			if(auto host = req->getHeader(http::Header::Host)){
				if(services.count(*host) > 0) return services[*host]->take(conn, req, respb);
				if(services2.count(*host) > 0) return services2[*host]->take(conn, req, respb);
			} else if(defolt) return (*defolt)->take(conn, req, respb);
			respb(http::Response(http::Status::NOT_FOUND, "No such service available"));
		}
};

R1otBuilder& R1otBuilder::addService(const std::string& n, SServer s){
	services[n] = s;
	return *this;
}
R1otBuilder& R1otBuilder::addService2(const std::string& n, SServer s){
	services2[n] = s;
	return *this;
}
R1otBuilder& R1otBuilder::setDefaultService(SServer s){
	defolt = s;
	return *this;
}
SServer R1otBuilder::build(){ return SServer(new R1oter(std::move(services), std::move(services2), std::move(defolt))); }

}