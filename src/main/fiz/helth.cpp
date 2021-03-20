#include "helth.hpp"

#include <http/rr.hpp>
#include <iostream>

namespace redips::fiz {

using namespace std::literals;
using namespace magikop;
using namespace yasync;
using namespace yasync::io;

HealthMonitor::HealthMonitor(IOYengine* e, TickTack::Duration i) : engine(e), interval(i) {}

void HealthMonitor::shutdown(){
	ticktack.shutdown();
}

struct Interim {
	TickTack::Id t = TickTack::UnId;
	TickTack::Id tM = TickTack::UnId;
	TickTack::Id tD = TickTack::UnId;
	NetworkedAddressInfo addr;
	IOResource conn;
	http::Request req;
	HealthMonitor::SAH sah;
	Interim(NetworkedAddressInfo && a, const std::string& path, HealthMonitor::SAH s) : addr(std::move(a)), req(http::Method::GET, path), sah(s) {}
};

template<int SDomain, int SType, int SProto, typename AddressInfo> HealthMonitor::SAH HealthMonitor::_add(NetworkedAddressInfo && addr, const std::string& path){
	const auto sah = std::make_shared<std::atomic<Health>>(Health::Missing);
	const auto interim = std::make_shared<Interim>(std::move(addr), path, sah);
	auto shr = [=](Health h){
		ticktack.stop(interim->tM);
		ticktack.stop(interim->tD);
		interim->tM = interim->tD = TickTack::UnId;
		sah->store(h);
	};
	auto conok = [=](){
		auto wr = interim->conn->writer();
		interim->req.write(wr);
		engine->engine <<= wr->eod() >> ([=](){
			engine->engine <<= http::Response::read(interim->conn) >> ([=](auto resp){
				if(resp->status == http::Status::OK) shr(Health::Alive);
				else {
					std::cerr << "Health check self reported not OK\n";
					shr(Health::Dead);
				}
			}|[=](auto err){
				std::cerr << "Health check read error: " << err.desc << "\n";
				interim->conn.reset();
				shr(Health::Dead);
			});
		}|[=](auto err){
			std::cerr << "Health check send error: " << err << "\n";
			interim->conn.reset();
			shr(Health::Dead);
		});
	};
	interim->t = ticktack.start(TickTack::Clock::now(), interval, [=](auto, bool cancel){
		if(!cancel){
			interim->tM = ticktack.sleep(std::chrono::milliseconds(500), [sah](auto, bool cancel){ if(!cancel) sah->store(Health::Missing); });
			interim->tD = ticktack.sleep(interval/10, [interim](auto, bool cancel){
				if(!cancel){
					if(interim->conn) interim->conn->cancel();
					interim->sah->store(Health::Dead);
				}
			});
			if(interim->conn) conok();
			else netConnectTo<SDomain, SType, SProto, AddressInfo>(engine, &interim->addr) >> ([=](auto fconn){
				engine->engine <<= fconn >> ([=](auto conn){
					interim->conn = conn;
					conok();
				}|[=](auto err){
					std::cerr << "Healt check connection establish failed: " << err << "\n";
					shr(Health::Dead);
				});
			}|[=](auto err){
				std::cerr << "Healt check connection establish failed: " << err << "\n";
				shr(Health::Dead);
			});
		}
	});
	return sah;
}

template<int SDomain, int SType, int SProto, typename AddressInfo> result<HealthMonitor::SAH, std::string> HealthMonitor::_findadd(const IPp& ipp, const std::string& path){
	auto naddr = NetworkedAddressInfo::find<SDomain, SType, SProto>(ipp.ip, ipp.portstr());
	if(naddr.isOk()) return _add<SDomain, SType, SProto, AddressInfo>(std::move(*naddr.ok()), path);
	else return *naddr.err();
}

result<HealthMonitor::SAH, std::string> HealthMonitor::add(const IPp& ipp, const std::string& path){
	if(isValidIPv6(ipp.ip)) return _findadd<AF_INET6, SOCK_STREAM, IPPROTO_TCP, sockaddr_in6>(ipp, path);
	else if(isValidIPv4(ipp.ip)) return _findadd<AF_INET, SOCK_STREAM, IPPROTO_TCP, sockaddr_in>(ipp, path);
	else return "Invalid IP"s;
}

}
