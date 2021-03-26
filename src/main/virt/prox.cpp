#include "prox.hpp"

namespace redips::virt {

using namespace yasync;
using namespace yasync::io;
using namespace magikop;
using namespace http;

class ProxyingServer : public IServer {
	yasync::Yengine* const engine;
	ProxyConnectionFactory cf;
	unsigned retries;
	public:
		ProxyingServer(yasync::Yengine* e, ProxyConnectionFactory && c, unsigned r) : engine(e), cf(std::move(c)), retries(r) {}
		void sprr(SharedRRaw rra, RespBack resp, unsigned tr){
			if(tr > retries) return resp(http::Response(http::Status::BAD_GATEWAY));
			using rDecay = result<http::SharedRRaw, http::RRReadError>;
			engine <<= cf() >> ([=](auto pair){
				RRaw rr(*rra);
				rr.setHeader(Header::Connection, "close");
				if(!rr.hasHeader(Header::Host)) rr.setHeader(Header::Host, pair.second.ip);
				auto conn = pair.first;
				auto wr = conn->writer();
				rr.write(wr);
				return wr->eod() >> ([=](){ return http::RRaw::read(conn); }|[=](auto err){
					std::cerr << "Proxy conn fwd failed: " << err << "\n";
					return yasync::completed(rDecay::Err(http::RRReadError("")));
				});
			}|[=](auto err){
				if(err == CONFAERREVERY1ISDED) return yasync::completed(rDecay::Err(http::RRReadError(CONFAERREVERY1ISDED)));
				std::cerr << "Proxy est conn failed: " << err << "\n";
				return yasync::completed(rDecay::Err(http::RRReadError("")));
			}) >> ([=](auto rrbwd){ return resp(std::move(*rrbwd)); }|[=](auto err){
				if(err.desc == CONFAERREVERY1ISDED) return resp(http::Response(http::Status::SERVICE_UNAVAILABLE));
				if(err.desc != "") std::cerr << "Proxy conn bwd failed: " << err.desc << "\n";
				return sprr(rra, resp, tr+1);
			});
		}
		void take(const ConnectionInfo& inf, SharedRRaw rr, RespBack resp) override {
			auto fwd = rr->getHeader(http::Header::Forwarded);
			rr->setHeader(http::Header::Forwarded, fwd.has_value() && *fwd != "" ? *fwd + "," + inf.to_string() : inf.to_string());
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
		void sprr(SharedRRaw rra, RespBack resp, unsigned tr){
			if(tr > retries) return resp(http::Response(http::Status::BAD_GATEWAY));
			using rDecay = result<http::SharedRRaw, http::RRReadError>;
			engine <<= cf() >> ([=](auto pair){
				RRaw rr(*rra);
				rr.setHeader(Header::Connection, "keep-alive");
				if(!rr.hasHeader(Header::Host)) rr.setHeader(Header::Host, pair.second.ip);
				auto conn = pair.first;
				auto wr = conn->writer();
				rr.write(wr);
				engine <<= wr->eod() >> ([=](){ return http::RRaw::read(conn); }|[=](auto err){
					return yasync::completed(rDecay::Err(http::RRReadError(err)));
				}) >> ([=](auto rrbwd){
					uf(ConnectionResult::Ok(conn));
					return resp(std::move(*rrbwd));
				}|[=](auto err){
					std::cerr << "Proxy rw failed: " << err.desc << "\n";
					uf(ConnectionResult::Err(err.desc));
					return sprr(rra, resp, tr+1);
				});
			}|[=](auto err){
				if(err == CONFAERREVERY1ISDED) return resp(http::Response(http::Status::SERVICE_UNAVAILABLE));
				std::cerr << "Proxy est conn failed: " << err << "\n";
				return sprr(rra, resp, tr+1);
			});
		}
		void take(const ConnectionInfo& inf, SharedRRaw rr, RespBack resp) override {
			auto fwd = rr->getHeader(http::Header::Forwarded);
			rr->setHeader(http::Header::Forwarded, fwd.has_value() && *fwd != "" ? *fwd + "," + inf.to_string() : inf.to_string());
			return sprr(rr, resp, 0);
		}
};

SServer proxyTo(yasync::Yengine* e, ProxyConnectionFactory && c, unsigned r){
	return SServer(new ProxyingServer(e, std::move(c), r));
}

}
