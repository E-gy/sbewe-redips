#include "prox.hpp"

#include <util/strim.hpp>

namespace redips::virt {

using namespace yasync;
using namespace yasync::io;
using namespace magikop;
using namespace http;

static void processFWD(const ConnectionInfo& inf, const SharedRRaw& rr){
	int fwhc = rr->hasHeader(Header::Forwarded) ? 10 : 0;
	fwhc += rr->hasHeader("X-Forwarded-For") ? 1 : 0;
	fwhc += rr->hasHeader("X-Forwarded-Host") ? 1 : 0;
	fwhc += rr->hasHeader("X-Forwarded-Proto") ? 1 : 0;
	if(fwhc == 1){ //transition
		auto konvert = [](const std::string& h, const std::string& pref){
			std::ostringstream fwd;
			for(std::size_t p = 0; p < h.length();){
				auto np = h.find(',', p);
				auto seg = h.substr(p, np-p);
				trim(seg);
				if(p > 0) fwd << ",";
				fwd << pref << "=";
				if(isValidIPv6(seg)) fwd << "\"[" << seg << "]\"";
				else if(seg.find(':') != seg.npos) fwd << "\"" << seg << "\"";
				else fwd << seg;
				if(np == h.npos) break;
				else p = np+1;
			}
			return fwd.str();
		};
		if(auto xfh = rr->getHeader("X-Forwarded-For")) rr->setHeader(Header::Forwarded, konvert(*xfh, "for"));
		else if(auto xfh = rr->getHeader("X-Forwarded-for")) rr->setHeader(Header::Forwarded, konvert(*xfh, "for"));
		else if(auto xfh = rr->getHeader("X-Forwarded-Host")) rr->setHeader(Header::Forwarded, konvert(*xfh, "host"));
		else if(auto xfh = rr->getHeader("X-Forwarded-host")) rr->setHeader(Header::Forwarded, konvert(*xfh, "host"));
		else if(auto xfh = rr->getHeader("X-Forwarded-Proto")) rr->setHeader(Header::Forwarded, konvert(*xfh, "proto"));
		else if(auto xfh = rr->getHeader("X-Forwarded-proto")) rr->setHeader(Header::Forwarded, konvert(*xfh, "proto"));
	}
	//append self
	auto fwd = rr->getHeader(http::Header::Forwarded);
	rr->setHeader(http::Header::Forwarded, fwd.has_value() && *fwd != "" ? *fwd + "," + inf.to_string() : inf.to_string());
}

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
				if(auto conn = rr.getHeader(Header::Connection)){
					auto hbhrem = *conn;
					for(std::size_t sp = 0; sp < hbhrem.length();){
						auto e = hbhrem.find(',', sp);
						auto hbhh = hbhrem.substr(sp, e == std::string::npos ? e : e-sp);
						trim(hbhh);
						rr.removeHeader(hbhh);
						sp = e == std::string::npos ? e : e+1;
					}
				}
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
			processFWD(inf, rr);
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
				if(auto conn = rr.getHeader(Header::Connection)){
					auto hbhrem = *conn;
					for(std::size_t sp = 0; sp < hbhrem.length();){
						auto e = hbhrem.find(',', sp);
						auto hbhh = hbhrem.substr(sp, e == std::string::npos ? e : e-sp);
						trim(hbhh);
						rr.removeHeader(hbhh);
						sp = e == std::string::npos ? e : e+1;
					}
				}
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
			processFWD(inf, rr);
			return sprr(rr, resp, 0);
		}
};

SServer proxyTo(yasync::Yengine* e, ProxyConnectionFactory && c, unsigned r){
	return SServer(new ProxyingServer(e, std::move(c), r));
}

}
