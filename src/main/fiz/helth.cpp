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
	TickTack::Id tEC = TickTack::UnId;
	NetworkedAddressInfo addr;
	std::shared_ptr<ConnectingSocket> econn;
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
		interim->tM = interim->tD = interim->tEC = TickTack::UnId;
		sah->store(h);
	};
	auto conok = [=](){
		std::cout << "Ping!\n";
		if(interval/10 > MISSINGT && sah->load() == Health::Alive) interim->tM = ticktack.sleep(MISSINGT, [sah](auto, bool cancel){ if(!cancel) sah->store(Health::Missing); });
		interim->tD = ticktack.sleep(interval/10, [interim](auto, bool cancel){
			if(!cancel){
				if(interim->conn){
					std::cout << "Ping timeout\n";
					interim->conn->cancel();
					interim->conn.reset();
				}
				interim->sah->store(Health::Dead);
			}
		});
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
				std::cout << "Connection lost\n";
				shr(Health::Dead);
			});
		}|[=](auto err){
			std::cerr << "Health check send error: " << err << "\n";
			interim->conn.reset();
			std::cout << "Connection lost\n";
			shr(Health::Dead);
		});
	};
	interim->t = ticktack.start(TickTack::Clock::now(), interval, [=](auto, bool cancel){
		if(!cancel){
			if(interim->conn) conok();
			else {
				std::cout << "Establishing connection...\n";
				netConnectTo<SDomain, SType, SProto, AddressInfo>(engine, &interim->addr) >> ([=](auto econns){
					interim->econn = econns;
					interim->tEC = ticktack.sleep(interval/10, [=](auto, bool cancel){
						if(!cancel){
							std::cout << "Connection establish failed (timeout)\n";
							if(interim->econn) interim->econn->cancel();
							else interim->sah->store(Health::Dead);
						} else if(interim->econn) interim->econn.reset();
					});
					engine->engine <<= econns->connest() >> ([=](auto conn){
						std::cout << "Conn OK!\n";
						ticktack.stop(interim->tEC);
						interim->econn.reset();
						interim->conn = conn;
						conok();
					}|[=](auto err){
						std::cerr << "Healt check connection establish failed: " << err << "\n";
						ticktack.stop(interim->tEC);
						interim->econn.reset();
						shr(Health::Dead);
					});
				}|[=](auto err){
					std::cerr << "Healt check connection establish failed: " << err << "\n";
					shr(Health::Dead);
				});
			}
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