#include "preqh.hpp"

#include <iostream>
#include <ctime>
#include <cstring>
#include <iomanip>

namespace redips::fiz {

using namespace magikop;
using namespace http;

inline bool keepAlive(RR& rr){
	if(auto conn = rr.getHeader(Header::Connection)) if(conn->find("close") != std::string::npos) return false;
	return true;
}

void ConnectionCare::setIdle(yasync::io::IOResource conn){
	std::unique_lock lok(idlok);
	idle.insert(conn);
}
void ConnectionCare::unsetIdle(yasync::io::IOResource conn){
	std::unique_lock lok(idlok);
	idle.erase(conn);
}

void ConnectionCare::shutdown(){
	sdown = true;
	std::unordered_set<yasync::io::IOResource> ridle;
	{
		std::unique_lock lok(idlok);
		std::swap(ridle, idle);
	}
	for(auto conn : ridle) conn->cancel();
}

void ConnectionCare::takeCare(ConnectionInfo inf, virt::SServer vs){
	auto conn = inf.connection;
	setIdle(conn);
	conn->engine <<= RRaw::read(conn) >> ([=](SharedRRaw reqwest){
		unsetIdle(conn);
		const bool kal = keepAlive(*reqwest);
		vs->take(ConnectionInfo{inf.connection, inf.address, inf.protocol, reqwest->getHeader(Header::Host)}, reqwest, [=](auto resp){
			auto wr = conn->writer();
			{
				auto t = std::time(nullptr);
				auto tm = *std::gmtime(&t); //FIXME not thread-safe
				resp.setHeader(Header::Date, std::put_time(&tm, "%a, %d %b %Y %H:%M:%S %Z"));
			}
			resp.setHeader(Header::Connection, kal && !sdown ? "keep-alive" : "closed");
			resp.write(wr);
			conn->engine <<= wr->eod() >> ([=](){
				if(kal && !sdown) takeCare(inf, vs); //If shut down is initiated after keep-alive is sent, oh well. May as well close it now.
			}|[](auto err){
				std::cerr << "Failed to respond: " << err << "\n";
			});
		});
	}|[](auto err){ std::cerr << "Read request error " << err.desc << "\n"; });
}

}
