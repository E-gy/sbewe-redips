#include "estconn.hpp"

namespace redips::fiz {

using namespace std::literals;

template<int SDomain, int SType, int SProto, typename AddressInfo> auto createConnectionFactory(yasync::io::NetworkedAddressInfo && addr, const IPp& ipp, yasync::io::IOYengine* engine, yasync::TickTack* ticktack, const yasync::TickTack::Duration& timeout, unsigned shora){
	return [addr = std::make_shared<yasync::io::NetworkedAddressInfo>(std::move(addr)), ipp, engine, ticktack, timeout, shora](){
		auto conne = std::make_shared<yasync::OutsideFuture<ConnectionFactoryResult>>();
		auto retry = [=](auto re, unsigned t) -> void {
			engine->engine <<= yasync::io::netConnectTo<SDomain, SType, SProto, AddressInfo>(engine, addr.get()) >> ([=](auto econns){
				auto tEC = ticktack->sleep(timeout, [=](auto, bool cancel){ if(!cancel) econns->cancel(); });
				return econns->connest() >> [=](auto connr){
					ticktack->stop(tEC);
					return connr.mapOk([&](auto conn){ return std::pair<yasync::io::IOResource, IPp>(conn, ipp); });
				};
			}|[](auto err){
				return yasync::completed(ConnectionFactoryResult::Err(err));
			}) >> [=](auto tr){
				if(tr.isOk() || t >= shora){
					conne->completed(std::move(tr));
					engine->engine->notify(conne);
				} else ticktack->sleep(timeout/15, [=](auto, bool cancel){
					if(!cancel) return re(re, t+1);
					else {
						conne->completed(ConnectionFactoryResult::Err("Retry cancelled lol"));
						engine->engine->notify(conne);
					}
				});
			};
		};
		retry(retry, 0);
		return conne;
	};
}

template<int SDomain, int SType, int SProto, typename AddressInfo> result<ConnectionFactory, std::string> _findConnectionFactory(const IPp& ipp, yasync::io::IOYengine* engine, yasync::TickTack* ticktack, const yasync::TickTack::Duration& timeout, unsigned shora){
	auto naddr = yasync::io::NetworkedAddressInfo::find<SDomain, SType, SProto>(ipp.ip, ipp.portstr());
	if(naddr.isOk()) return ConnectionFactory(createConnectionFactory<SDomain, SType, SProto, AddressInfo>(std::move(*naddr.ok()), ipp, engine, ticktack, timeout, shora));
	else return *naddr.err();
}

result<ConnectionFactory, std::string> findConnectionFactory(const IPp& ipp, yasync::io::IOYengine* engine, yasync::TickTack* ticktack, const yasync::TickTack::Duration& timeout, unsigned shora){
	if(isValidIPv6(ipp.ip)) return _findConnectionFactory<AF_INET6, SOCK_STREAM, IPPROTO_TCP, sockaddr_in6>(ipp, engine, ticktack, timeout, shora);
	else if(isValidIPv4(ipp.ip)) return _findConnectionFactory<AF_INET, SOCK_STREAM, IPPROTO_TCP, sockaddr_in>(ipp, engine, ticktack, timeout, shora);
	else return "Invalid IP"s;
}

}
