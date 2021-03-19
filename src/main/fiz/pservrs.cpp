#include <yasync/iosock.hpp>
#include "pservrs.hpp"

#include <ossl/iores.hpp>

#include <string>
#include <iostream>

namespace redips::fiz {

using namespace std::literals;
using namespace magikop;
using namespace yasync::io::ssl;

class P2VLSNIHandler;
template<typename LiSo> class P2VListener : public IListener {
	public:
		LiSo liso;
		yasync::Future<void> liend;
		SharedSSLContext acctxt; //captured to last for as long as the listener lasts
		std::shared_ptr<P2VLSNIHandler> sniha;
		P2VListener(LiSo s, yasync::Future<void> e, SharedSSLContext acc, std::shared_ptr<P2VLSNIHandler> sni) : liso(s), liend(e), acctxt(acc), sniha(sni) {}
		~P2VListener() = default;
		yasync::Future<void> onShutdown() override { return liend; };
		void shutdown() override { liso->shutdown(); }
};

class P2VLSNIHandler {
	HostMapper<SharedSSLContext> hmap;
	public:
		P2VLSNIHandler(const HostMapper<SharedSSLContext>& hm) : hmap(hm) {}
		int handleSNI(SSL* ssl, [[maybe_unused]] int* al){
			auto snam = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
			if(snam) if(auto host = hmap.resolve(snam)){
				SSL_set_SSL_CTX(ssl, (*host)->ctx());
				return SSL_TLSEXT_ERR_OK;
			}
			return SSL_TLSEXT_ERR_NOACK; //TODO
		}
};

int handleSNI(SSL* ssl, int* al, void* listener){
	return reinterpret_cast<P2VLSNIHandler*>(listener)->handleSNI(ssl, al);
}

template<int SDomain, int SType, int SProto, typename AddressInfo> result<SListener, std::string> listenOnV(yasync::io::IOYengine* engine, const IPp& ipp, const HostMapper<SharedSSLContext>& hmap, ConnectionCare* cc, virt::SServer vs){
	auto ar = yasync::io::NetworkedAddressInfo::find<SDomain, SType, SProto>(ipp.ip, ipp.portstr());
	if(auto err = ar.err()) return *err;
	auto acctxr = yasync::io::ssl::createSSLContext();
	if(auto err = acctxr.err()) return *err;
	auto acctx = *acctxr.ok();
	auto sniha = std::make_shared<P2VLSNIHandler>(hmap);
	SSL_CTX_set_tlsext_servername_callback(acctx->ctx(), handleSNI);
	SSL_CTX_set_tlsext_servername_arg(acctx->ctx(), sniha.get());
	auto lir1 = yasync::io::netListen<SDomain, SType, SProto, AddressInfo>(engine, []([[maybe_unused]] auto _, yasync::io::sysneterr_t err, const std::string& location){
		std::cerr << "Listen error: " << yasync::io::printSysNetError(location, err) << "\n";
		return false;
	}, [acctx, cc, vs]([[maybe_unused]] auto _, const yasync::io::IOResource& conn){
		auto ssl = SSL_new(acctx->ctx());
		if(!ssl){
			std::cerr << "Failed to instantiate SSL\n";
			return;
		}
		openSSLIO(conn, ssl) >> ([cc, vs, ssl](auto conn){
			SSL_set_accept_state(ssl);
			cc->takeCare(conn, vs);
		}|[](auto err){ std::cerr << "Connection convert to SSL error " << err << "\n"; });
	});
	if(auto err = lir1.err()) return *err;
	auto listener = std::move(*lir1.ok());
	auto lir2 = listener->listen(ar.ok());
	if(auto err = lir2.err()) return *err;
	return std::shared_ptr<IListener>(new P2VListener<decltype(listener)>(listener, *lir2.ok(), acctx, sniha));
}

result<SListener, std::string> listenOnSecure(yasync::io::IOYengine* e, const IPp& ipp, const HostMapper<SharedSSLContext>& hmap, ConnectionCare* cc, virt::SServer vs){
	if(!ipp.isValidPort()) return "Invalid port"s;
	if(ipp.isValidIPv6()) return listenOnV<AF_INET6, SOCK_STREAM, IPPROTO_TCP, sockaddr_in6>(e, ipp, hmap, cc, vs);
	else if(ipp.isValidIPv4()) return listenOnV<AF_INET, SOCK_STREAM, IPPROTO_TCP, sockaddr_in>(e, ipp, hmap, cc, vs);
	else return "Invalid address"s;
}

}
