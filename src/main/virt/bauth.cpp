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
	public:
		BasicAuther(const std::string& r, std::unordered_set<std::string>&& c, SServer p) : realm(r), creds(c), protege(p) {}
		void take(yasync::io::IOResource conn, redips::http::SharedRRaw req, RespBack respb) override {
			if(auto auh = req->getHeader(Header::Authorization)){
				std::istringstream val(*auh);
				std::string type, cred64;
				if(val >> type >> cred64) if(type == "Basic" && creds.count(b64decode(cred64.c_str(), cred64.length())) > 0) return protege->take(conn, req, respb);
			}
			Response resp(Status::UNAUTHORIZED);
			resp.setHeader(Header::Authenticate, "Basic realm=\""+realm+"\"");
			respb(std::move(resp));
		}
};

SServer putBehindBasicAuth(const std::string& realm, const std::vector<std::string>& credentials, SServer protege){
	return SServer(new BasicAuther(realm, std::unordered_set<std::string>(credentials.begin(), credentials.end()), protege));
}

}
