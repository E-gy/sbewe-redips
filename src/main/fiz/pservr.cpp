#include <yasync/iosock.hpp>
#include "pservr.hpp"

#include <string>
#include <iostream>

namespace redips::fiz {

using namespace std::literals;
using namespace magikop;

template<typename LiSo> class P2VListener : public IListener {
	public:
		LiSo liso;
		yasync::Future<void> liend;
		P2VListener(LiSo s, yasync::Future<void> e) : liso(s), liend(e) {}
		~P2VListener() = default;
		yasync::Future<void> onShutdown() override { return liend; };
		void shutdown() override { liso->shutdown(); }
};

template<int SDomain, int SType, int SProto, typename AddressInfo> result<SListener, std::string> listenOnV(yasync::io::IOYengine* engine, const IPp& ipp, ConnectionCare* cc, virt::SServer vs){
	auto ar = yasync::io::NetworkedAddressInfo::find<SDomain, SType, SProto>(ipp.ip, ipp.portstr());
	if(auto err = ar.err()) return *err;
	auto lir1 = yasync::io::netListen<SDomain, SType, SProto, AddressInfo>(engine, []([[maybe_unused]] auto _, yasync::io::sysneterr_t err, const std::string& location){
		std::cerr << "Listen error: " << yasync::io::printSysNetError(location, err) << "\n";
		return false;
	}, [=]([[maybe_unused]] auto _, const yasync::io::IOResource& conn){
		cc->takeCare(conn, vs);
	});
	if(auto err = lir1.err()) return *err;
	auto listener = std::move(*lir1.ok());
	auto lir2 = listener->listen(ar.ok());
	if(auto err = lir2.err()) return *err;
	return std::shared_ptr<IListener>(new P2VListener<decltype(listener)>(listener, *lir2.ok()));
}

result<SListener, std::string> listenOn(yasync::io::IOYengine* e, const IPp& ipp, ConnectionCare* cc, virt::SServer vs){
	if(!ipp.isValidPort()) return "Invalid port"s;
	if(ipp.isValidIPv6()) return listenOnV<AF_INET6, SOCK_STREAM, IPPROTO_TCP, sockaddr_in6>(e, ipp, cc, vs);
	else if(ipp.isValidIPv4()) return listenOnV<AF_INET, SOCK_STREAM, IPPROTO_TCP, sockaddr_in>(e, ipp, cc, vs);
	else return "Invalid address"s;
}

}
