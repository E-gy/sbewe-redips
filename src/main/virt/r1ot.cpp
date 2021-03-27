#include "r1ot.hpp"

#include <iostream>

namespace redips::virt {

using namespace magikop;

class R1oter : public IServer {
	HostMapper<SServer> services;
	std::optional<SServer> defolt;
	R1oter(HostMapper<SServer> && s, std::optional<SServer> && d) : services(s), defolt(d) {}
	public:
		friend class R1otBuilder;
		void take(const ConnectionInfo& conn, redips::http::SharedRRaw req, RespBack respb) override {
			if(auto host = req->getHeader(http::Header::Host)) if(auto serv = services.resolve(*host)) return (*serv)->take(conn, req, respb);
			if(defolt) return (*defolt)->take(conn, req, respb);
			return respb(http::Response(http::Status::BAD_REQUEST));
		}
};

R1otBuilder& R1otBuilder::addService(const HostK& k, SServer s){
	services.addHost(s, k);
	return *this;
}
R1otBuilder& R1otBuilder::setDefaultService(SServer s){
	defolt = s;
	return *this;
}
SServer R1otBuilder::build(){ return SServer(new R1oter(std::move(services), std::move(defolt))); }

}