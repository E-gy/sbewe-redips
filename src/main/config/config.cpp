#include "config.hpp"

#include <algorithm>
#include <sstream>
#include <util/json.hpp>
#include <util/filetype.hpp>
#include <ossl/ssl.hpp>
#include <openssl/x509v3.h>

using nlohmann::json;

void to_json(json& j, const IPp& ip){
	j["ip"] = ip.ip;
	j["port"] = ip.port;
}

void from_json(const json& j, IPp& ip){
	j.at("ip").get_to(ip.ip);
	j.at("port").get_to(ip.port);
	if(!isValidIP(ip.ip)) throw json::type_error::create(301, "Address string is not a valid ip address");
	if(!isValidPort(ip.port)) throw json::type_error::create(301, "Port number out of bounds");
}

namespace redips::config {

// helper type for the visitor
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

std::string VHost::identk(){
	std::ostringstream k;
	k << '[' << (this->address.ip == "127.0.0.1" ? "0.0.0.0" : this->address.ip) << "]:" << this->address.port << "#" << this->serverName;
	return k.str();
}

HostK VHost::tok(){
	return HostK{ serverName, address.ip, address.port };
}

/*
void to_json(json& j, const T& t){}
void from_json(const json& j, T& t){}
*/

void to_json(json& j, const Timeout& d){
	j = d.seconds;
}
void from_json(const json& j, Timeout& d){
	j.get_to(d.seconds);
	if(d.seconds == 0) throw json::type_error::create(301, "A timeout can't be 0");
}

void to_json(json& j, const TLS& tls){
	j["ssl_cert"] = tls.cert;
	j["ssl_key"] = tls.key;
}
void from_json(const json& j, TLS& tls, const std::string& host){
	j.at("ssl_cert").get_to(tls.cert);
	j.at("ssl_key").get_to(tls.key);
	if(getFileType(tls.cert) != FileType::File) throw json::type_error::create(301, "Specified TLS cert file does not exist");
	if(getFileType(tls.key) != FileType::File) throw json::type_error::create(301, "Specified TLS key file does not exist");
	auto sslr = yasync::io::ssl::createSSLContext(tls.cert, tls.key);
	if(auto sslerr = sslr.err()) throw json::type_error::create(301, "TLS Credentials invalid: " + *sslerr);
	auto x509 = ::SSL_CTX_get0_certificate((*sslr.ok())->ctx());
	if(::X509_check_host(x509, host.c_str(), host.length(), 0, nullptr) != 1) throw json::type_error::create(301, "TLS Certificate host name mismatch");
}

void to_json(json& j, const BasicAuth& auth){
	j["auth_basic"] = auth.realm;
	j["auth_basic_users"] = auth.credentials;
}
void from_json(const json& j, BasicAuth& auth){
	j.at("auth_basic").get_to(auth.realm);
	j.at("auth_basic_users").get_to(auth.credentials);
	if(auth.realm.find('"') != std::string::npos) throw json::type_error::create(301, "Auth realm must not contain quotation marks");
	for(auto c : auth.credentials){
		auto sep = c.find(':');
		if(sep == std::string::npos) throw json::type_error::create(301, "User credential must be of shape `${username}:${password}");
		if(sep == 0) throw json::type_error::create(301, "User credential must be of shape `${username}:${password}` with non-empty username");
	}
}

void to_json(json& j, const FileServer& fs){
	j["root"] = fs.root;
	if(fs.defaultFile) j["defaultFile"] = *fs.defaultFile;
	if(fs.autoindex) j["auto_index"] = *fs.autoindex;
}
void from_json(const json& j, FileServer& fs){
	j.at("root").get_to(fs.root);
	if(j.contains("default_file")) fs.defaultFile = j.at("default_file").get<std::string>();
	if(j.contains("auto_index")) fs.autoindex = j.at("auto_index").get<bool>();
}

void to_json(json& j, const std::string& pref, const HeadMod& hm){
	if(hm.add.size() > 0) j[pref + "set_header"] = hm.add;
	if(hm.remove.size() > 0) j[pref + "remove_header"] = hm.remove;
	if(hm.rename.size() > 0) j[pref + "rename_header"] = hm.rename;
}
void from_json(const json& j, const std::string& pref, HeadMod& hm){
	if(j.contains(pref + "set_header")) j.at(pref + "set_header").get_to(hm.add);
	if(j.contains(pref + "remove_header")) j.at(pref + "remove_header").get_to(hm.remove);
	if(j.contains(pref + "rename_header")) j.at(pref + "rename_header").get_to(hm.rename);
}

void to_json(json& j, const Proxy& p){
	std::visit(overloaded {
		[&](const std::string& upstr){ j["upstream"] = upstr; },
		[&](const IPp& ip){ to_json(j, ip); },
	}, p.upstream);
	to_json(j, "proxy_", p.fwdMod);
	to_json(j, "", p.bwdMod);
	if(p.transactionTimeout) j["timeout"] = *p.transactionTimeout;
}
void from_json(const json& j, Proxy& p){
	if(j.contains("upstream"))
		if(j.contains("ip") || j.contains("port")) throw json::type_error::create(301, "Both anonymous and named upstream specified");
		else p.upstream = j.at("upstream").get<std::string>();
	else p.upstream = j.get<IPp>();
	from_json(j, "proxy_", p.fwdMod);
	from_json(j, "", p.bwdMod);
	if(j.contains("timeout")) p.transactionTimeout = j.at("timeout").get<Timeout>();
}

void to_json(json& j, const VHost& vh){
	to_json(j, vh.address);
	j["server_name"] = vh.serverName;
	std::visit(overloaded {
		[&](const FileServer& mi){ to_json(j, mi); },
		[&](const Proxy& mi){ j["proxy_pass"] = mi; },
	}, vh.mode);
	if(vh.tls) to_json(j, *vh.tls);
	if(vh.auth) to_json(j, *vh.auth);
	if(vh.isDefault) j["default_vhost"] = true;
}

void from_json(const json& j, VHost& vh){
	j.get_to(vh.address);
	j.at("server_name").get_to(vh.serverName);
	if(j.contains("proxy_pass"))
		if(j.contains("root") || j.contains("defaultFile") || j.contains("auto_index")) throw json::type_error::create(301, "Both file server and proxied mode specified");
		else vh.mode = j.at("proxy_pass").get<Proxy>();
	else vh.mode = j.get<FileServer>();
	if(j.contains("ssl_cert") || j.contains("ssl_key")){
		TLS tls;
		from_json(j, tls, vh.serverName);
		vh.tls = tls;
	}
	if(j.contains("auth_basic") || j.contains("auth_basic_users")) vh.auth = j.get<BasicAuth>();
	if(j.contains("default_vhost")) j.at("default_vhost").get_to(vh.isDefault);
}

void to_json(json& j, const LoadBalancingMethod& lbm){
	switch(lbm){
		case LoadBalancingMethod::RoundRobin: j = "round-robin"; break;
		case LoadBalancingMethod::FailOver: j = "failover"; break;
		case LoadBalancingMethod::FailRobin: j = "fail-robin"; break;
		default: break;
	}
}
void from_json(const json& j, LoadBalancingMethod& lbm){
	std::string str = j;
	if(str == "round-robin"){ lbm = LoadBalancingMethod::RoundRobin; return; }
	if(str == "failover"){ lbm = LoadBalancingMethod::FailOver; return; }
	if(str == "fail-robin"){ lbm = LoadBalancingMethod::FailRobin; return; }
	throw json::type_error::create(301, "Unknown load balancing method '" + str + "'");
}

void to_json(json& j, const Uphost& uh){
	to_json(j, uh.address);
	if(uh.weight) j["weight"] = *uh.weight;
	if(uh.healthCheckUrl) j["health"] = *uh.healthCheckUrl;
}
void from_json(const json& j, Uphost& uh){
	j.get_to(uh.address);
	if(j.contains("weight")) if(*(uh.weight = j.at("weight").get<unsigned>()) == 0) throw json::type_error::create(301, "Weight can't be 0");
	if(j.contains("health")){
		uh.healthCheckUrl = j.at("health").get<std::string>();
		if(uh.healthCheckUrl->length() == 0 || (*uh.healthCheckUrl)[0] != '/') throw json::type_error::create(301, "Invalid health check url");
	}
}

void to_json(json& j, const Upstream& u){
	to_json(j["method"], u.lbm);
	j["hosts"] = u.hosts;
	if(u.retries) j["retries"] = *u.retries;
}
void from_json(const json& j, Upstream& u){
	from_json(j.at("method"), u.lbm);
	j.at("hosts").get_to(u.hosts);
	if(j.contains("retries")) u.retries = j.at("retries").get<unsigned>();
	if(u.hosts.size() == 0) throw json::type_error::create(301, "Upstream must have at least one uphost");
	if(u.lbm == LoadBalancingMethod::FailOver || u.lbm == LoadBalancingMethod::FailRobin) if(std::count_if(u.hosts.begin(), u.hosts.end(), [](auto uh){ return !uh.healthCheckUrl.has_value(); }) > 0) throw json::type_error::create(301, "Select load balancing method requires all hosts to specify health check URL");
}

void to_json(json& j, const ThroughputUnlimiter& t){
	j["throughput_val"] = t.b;
	j["throughput_time"] = t.t;
}
void from_json(const json& j, ThroughputUnlimiter& t){
	j.at("throughput_val").get_to(t.b);
	j.at("throughput_time").get_to(t.t);
	if(t.t < 1E-2) throw json::type_error::create(301, "Throughput interval too small");
	if(t.b == 0) throw json::type_error::create(301, "Throughput rate 0 is useless");
}

void to_json(json& j, const Timings& t){
	if(t.keepAlive) j["keep_alive"] = *t.keepAlive;
	if(t.transaction) j["transaction"] = *t.transaction;
	if(t.throughput) to_json(j, *t.throughput);
}
void from_json(const json& j, Timings& t){
	if(j.contains("keep_alive")) t.keepAlive = j.at("keep_alive").get<Timeout>();
	if(j.contains("transaction")) t.keepAlive = j.at("transaction").get<Timeout>();
	if(j.contains("throughput_val") || j.contains("throughput_time")){
		ThroughputUnlimiter ut;
		from_json(j, ut);
		t.throughput = ut;
	}
}

void to_json(json& j, const Config& c){
	j["vhosts"] = c.vhosts;
	if(c.upstreams.size() > 0) j["upstreams"] = c.upstreams;
	if(c.timings.keepAlive || c.timings.throughput || c.timings.transaction) j["timeout"] = c.timings;
}

void from_json(const json& j, Config& c){
	j.at("vhosts").get_to(c.vhosts);
	if(std::count_if(c.vhosts.begin(), c.vhosts.end(), [](auto vh){ return vh.isDefault; }) > 1) throw json::type_error::create(301, "Multiple VHosts marked as default");
	std::unordered_set<std::string> iks;
	for(auto vh : c.vhosts) if(!iks.insert(vh.identk()).second) throw json::type_error::create(301, "Some VHosts are not differentiable");
	if(j.contains("upstreams")) j.at("upstreams").get_to(c.upstreams);
	for(auto vh : c.vhosts) std::visit(overloaded {
		[&](const FileServer&){ /*TODO check root exists..?*/ },
		[&](const Proxy& prox){ std::visit(overloaded {
			[&](const std::string& ups){ if(c.upstreams.find(ups) == c.upstreams.end()) throw json::type_error::create(301, "VH proxy named upstream specified, but not found (" + ups + ")"); },
			[&](const IPp&){},
		}, prox.upstream); },
	}, vh.mode);
	if(j.contains("timeout")) j.at("timeout").get_to(c.timings);
}

result<Config, std::string> parse(std::istream& is){
	try {
		json j;
		is >> j;
		return j.get<Config>();
	} catch(json::exception& exc){
		return std::string(exc.what());
	}
}
result<void, std::string> serialize(std::ostream& os, const Config& cfg){
	try {
		json j(cfg);
		os << j;
		return result<void, std::string>::Ok();
	} catch(json::exception& exc){
		return std::string(exc.what());
	}
}

}

