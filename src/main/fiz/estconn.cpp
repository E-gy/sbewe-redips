#include "estconn.hpp"

namespace redips::fiz {

using namespace std::literals;

template<int SDomain, int SType, int SProto, typename AddressInfo> result<ConnectionFactory, std::string> _findConnectionFactory(const IPp& ipp, yasync::io::IOYengine* engine, yasync::TickTack* ticktack, const yasync::TickTack::Duration& timeout){
	auto naddr = yasync::io::NetworkedAddressInfo::find<SDomain, SType, SProto>(ipp.ip, ipp.portstr());
	if(naddr.isOk()) return ConnectionFactory(createConnectionFactory<SDomain, SType, SProto, AddressInfo>(std::move(*naddr.ok()), engine, ticktack, timeout));
	else return *naddr.err();
}

result<ConnectionFactory, std::string> findConnectionFactory(const IPp& ipp, yasync::io::IOYengine* engine, yasync::TickTack* ticktack, const yasync::TickTack::Duration& timeout){
	if(isValidIPv6(ipp.ip)) return _findConnectionFactory<AF_INET6, SOCK_STREAM, IPPROTO_TCP, sockaddr_in6>(ipp, engine, ticktack, timeout);
	else if(isValidIPv4(ipp.ip)) return _findConnectionFactory<AF_INET, SOCK_STREAM, IPPROTO_TCP, sockaddr_in>(ipp, engine, ticktack, timeout);
	else return "Invalid IP"s;
}

}
