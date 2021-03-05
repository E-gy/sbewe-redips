#include "config.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <util/json.hpp>
#include <util/ip.hpp>

using nlohmann::json;

namespace redips::config {

std::string VHost::identk(){
	std::ostringstream k;
	k << '[' << (this->ip == "127.0.0.1" ? "0.0.0.0" : this->ip) << "]:" << this->port << "#" << this->serverName;
	return k.str();
}

void to_json(json& j, const VHost& vh){
	j = json{{"ip", vh.ip},{"port", vh.port},{"server_name", vh.serverName},{"root", vh.root}};
	if(vh.defaultFile) j["default_file"] = vh.defaultFile.value();
}

void from_json(const json& j, VHost& vh){
	j.at("ip").get_to(vh.ip);
	j.at("port").get_to(vh.port);
	j.at("server_name").get_to(vh.serverName);
	j.at("root").get_to(vh.root);
	if(j.contains("default_file")) vh.defaultFile = j.at("default_file").get<std::string>();
	if(!isValidIP(vh.ip)) throw json::type_error::create(301, "Address string is not a valid ip address");
	if(!isValidPort(vh.port)) throw json::type_error::create(301, "Port number out of bounds");
	if(j.contains("ssl_cert") != j.contains("ssl_key")) throw json::type_error::create(301, "Incomplete SSL config");
	if(j.contains("ssl_cert")){
		SSL ssl;
		j.at("ssl_cert").get_to(ssl.cert);
		j.at("ssl_key").get_to(ssl.key);
		vh.ssl = ssl;
	}
	if(j.contains("auth_basic") != j.contains("auth_basic_users")) throw json::type_error::create(301, "Incomplete Basic Auth config");
	if(j.contains("auth_basic")){
		BasicAuth auth;
		j.at("auth_basic").get_to(auth.realm);
		j.at("auth_basic_users").get_to(auth.credentials);
		vh.auth = auth;
	}
}

void to_json(json& j, const Config& c){
	j = json{{"vhosts", c.vhosts}};
}

void from_json(const json& j, Config& c){
	j.at("vhosts").get_to(c.vhosts);
	if(std::count_if(c.vhosts.begin(), c.vhosts.end(), [](auto vh){ return vh.isDefault; }) > 1) throw json::type_error::create(301, "Multiple VHosts marked as default");
	std::unordered_set<std::string> iks;
	for(auto vh : c.vhosts) if(!iks.insert(vh.identk()).second) throw json::type_error::create(301, "Some VHosts are not differentiable");
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

