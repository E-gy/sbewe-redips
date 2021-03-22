#include "bauth.hpp"

#include <util/base64.hpp>
#include <sstream>
#include <unordered_set>

namespace redips::virt {

using namespace magikop;
using namespace redips::http;

class BasicAuther : public IServer {
	std::string realm;
	std::unordered_set<std::string> creds;
	SServer protege;
	bool proxh;
	public:
		BasicAuther(const std::string& r, std::unordered_set<std::string>&& c, SServer p, bool prox) : realm(r), creds(c), protege(p), proxh(prox) {}
		void take(yasync::io::IOResource conn, redips::http::SharedRRaw req, RespBack respb) override {
			if(auto auh = req->getHeader(proxh ? Header::ProxyAuthorization : Header::Authorization)){
				std::istringstream val(*auh);
				std::string type, cred64;
				if(val >> type >> cred64) if(type == "Basic" && creds.count(b64decode(cred64.c_str(), cred64.length())) > 0) return protege->take(conn, req, respb);
			}
			Response resp(proxh ? Status::PROXY_AUTHENTICATION_REQUIRED : Status::UNAUTHORIZED);
			resp.setHeader(proxh ? Header::ProxyAuthenticate : Header::Authenticate, "Basic realm=\""+realm+"\"");
			respb(std::move(resp));
		}
};

SServer putBehindBasicAuth(const std::string& realm, const std::vector<std::string>& credentials, SServer protege, bool proxh){
	return SServer(new BasicAuther(realm, std::unordered_set<std::string>(credentials.begin(), credentials.end()), protege, proxh));
}

}
