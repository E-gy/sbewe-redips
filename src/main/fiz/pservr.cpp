#include <yasync/iosock.hpp>
#include "pservr.hpp"

#include <util/ip.hpp>

#include <string>
#include <iostream>

using namespace std::literals;
using namespace magikop;

namespace redips::fiz {

template<typename LiSo> class P2VListener : public IListener {
	public:
		LiSo liso;
		yasync::Future<void> liend;
		P2VListener(LiSo s, yasync::Future<void> e) : liso(s), liend(e) {}
		~P2VListener() = default;
		yasync::Future<void> onShutdown() override { return liend; };
		void shutdown() override { liso->shutdown(); }
};

template<int SDomain, int SType, int SProto, typename AddressInfo> result<SListener, std::string> listenOnV(yasync::io::IOYengine* engine, const std::string& addr, unsigned port, virt::SServer vs){
	auto ar = yasync::io::NetworkedAddressInfo::find<SDomain, SType, SProto>(addr, std::to_string(port));
	if(auto err = ar.err()) return *err;
	auto lir1 = yasync::io::netListen<SDomain, SType, SProto, AddressInfo>(engine, []([[maybe_unused]] auto _, yasync::io::sysneterr_t err, const std::string& location){
		std::cerr << "Listen error: " << yasync::io::printSysNetError(location, err) << "\n";
		return false;
	}, [=]([[maybe_unused]] auto _, const yasync::io::IOResource& conn){
		conn->engine->engine <<= http::Request::read(conn) >> ([=](http::SharedRequest reqwest){ vs->take(conn, reqwest); }|[](auto err){ std::cerr << "Read request error " << err.desc << "\n"; });
	});
	if(auto err = lir1.err()) return *err;
	auto listener = std::move(*lir1.ok());
	auto lir2 = listener->listen(ar.ok());
	if(auto err = lir2.err()) return *err;
	return std::shared_ptr<IListener>(new P2VListener(listener, *lir2.ok()));
}

result<SListener, std::string> listenOn(yasync::io::IOYengine* e, const std::string& addr, unsigned port, virt::SServer vs){
	if(!isValidPort(port)) return "Invalid port"s;
	if(isValidIPv6(addr)) return listenOnV<AF_INET6, SOCK_STREAM, IPPROTO_TCP, sockaddr_in6>(e, addr, port, vs);
	else if(isValidIPv4(addr)) return listenOnV<AF_INET, SOCK_STREAM, IPPROTO_TCP, sockaddr_in>(e, addr, port, vs);
	else return "Invalid address"s;
}

}
