#include "headmod.hpp"

namespace redips::virt {

using namespace magikop;
using namespace redips::http;

void headMod(RR& rr, const HeadMod& mod){
	for(auto rem : mod.remove) rr.headers.erase(rem);
	for(auto ren : mod.rename){
		auto h = rr.headers.find(ren.first);
		if(h != rr.headers.end()){
			auto hv = h->second;
			rr.headers.erase(h);
			rr.headers[ren.second] = hv;
		}
	}
	for(auto add : mod.add) rr.setHeader(add.first, add.second);
}

class HeadersModifier : public IServer {
	HeadMod fwd, bwd;
	SServer prox;
	public:
		HeadersModifier(const HeadMod& f, const HeadMod& b, SServer p) : fwd(f), bwd(b), prox(p) {}
		void take(yasync::io::IOResource conn, redips::http::SharedRRaw req, RespBack respb) override {
			headMod(*req, fwd);
			return prox->take(conn, req, [=](auto resp){
				headMod(resp, bwd);
				respb(std::move(resp));
			});
		}
};

SServer modifyHeaders2(const HeadMod& fwd, const HeadMod& bwd, SServer prox){
	return SServer(new HeadersModifier(fwd, bwd, prox));
}

}
