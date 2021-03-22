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
		void take(const ConnectionInfo&, redips::http::SharedRRaw rr, RespBack resp) override {
			rr->setHeader(http::Header::Connection, "close");
			return sprr(rr, resp, 0);
		}
};

class PooledProxyingServer : public IServer {
	yasync::Yengine* const engine;
	ProxyConnectionFactory cf;
	ProxyConnectionUnfactory uf;
	unsigned retries;
	public:
		PooledProxyingServer(yasync::Yengine* e, ProxyConnectionFactory && c, ProxyConnectionUnfactory && u, unsigned r) : engine(e), cf(std::move(c)), uf(std::move(u)), retries(r) {}
		void sprr(redips::http::SharedRRaw rr, RespBack resp, unsigned tr){
			if(tr > retries) return resp(http::Response(http::Status::SERVICE_UNAVAILABLE));
			using rDecay = result<http::SharedRRaw, http::RRReadError>;
			engine <<= cf() >> ([=](auto conn){
				auto wr = conn->writer();
				rr->write(wr);
				engine <<= wr->eod() >> ([=](){ return http::RRaw::read(conn); }|[=](auto err){
					return yasync::completed(rDecay::Err(http::RRReadError(err)));
				}) >> ([=](auto rrbwd){
					uf(ConnectionResult::Ok(conn));
					return resp(std::move(*rrbwd));
				}|[=](auto err){
					std::cerr << "Proxy rw failed: " << err.desc << "\n";
					uf(ConnectionResult::Err(err.desc));
					return sprr(rr, resp, tr+1);
				});
			}|[=](auto err){
				std::cerr << "Proxy est conn failed: " << err << "\n";
				return sprr(rr, resp, tr+1);
			});
		}
		void take(const ConnectionInfo&, redips::http::SharedRRaw rr, RespBack resp) override {
			rr->setHeader(http::Header::Connection, "keep-alive");
			return sprr(rr, resp, 0);
		}
};

SServer proxyTo(yasync::Yengine* e, ProxyConnectionFactory && c, unsigned r){
	return SServer(new ProxyingServer(e, std::move(c), r));
}

}
