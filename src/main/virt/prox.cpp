#include "prox.hpp"

namespace redips::virt {

using namespace yasync;
using namespace yasync::io;
using namespace magikop;

class ProxyingServer : public IServer {
	yasync::Yengine* const engine;
	ProxyConnectionFactory cf;
	unsigned retries;
	public:
		ProxyingServer(yasync::Yengine* e, ProxyConnectionFactory && c, unsigned r) : engine(e), cf(std::move(c)), retries(r) {}
		void sprr(redips::http::SharedRRaw rr, RespBack resp, unsigned tr){
			if(tr > retries) return resp(http::Response(http::Status::SERVICE_UNAVAILABLE));
			using rDecay = result<http::SharedRRaw, http::RRReadError>;
			engine <<= cf() >> ([=](auto conn){
				auto wr = conn->writer();
				rr->write(wr);
				return wr->eod() >> ([=](){ return http::RRaw::read(conn); }|[=](auto err){
					std::cerr << "Proxy conn fwd failed: " << err << "\n";
					return yasync::completed(rDecay::Err(http::RRReadError("")));
				});
			}|[=](auto err){
				std::cerr << "Proxy est conn failed: " << err << "\n";
				return yasync::completed(rDecay::Err(http::RRReadError("")));
			}) >> ([=](auto rrbwd){ return resp(std::move(*rrbwd)); }|[=](auto err){
				if(err.desc != "") std::cerr << "Proxy conn bwd failed: " << err.desc << "\n";
				return sprr(rr, resp, tr+1);
			});
		}
		void take(yasync::io::IOResource, redips::http::SharedRRaw rr, RespBack resp) override {
			rr->setHeader(http::Header::Connection, "close");
			return sprr(rr, resp, 0);
		}
};

SServer proxyTo(yasync::Yengine* e, ProxyConnectionFactory && c, unsigned r){
	return SServer(new ProxyingServer(e, std::move(c), r));
}

}
