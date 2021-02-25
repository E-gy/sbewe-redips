#include "config.hpp"

#include <util/json.hpp>

using nlohmann::json;

namespace redips::config {

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
	//TODO validate
}

void to_json(json& j, const Config& c){
	j = json{{"vhosts", c.vhosts}};
}

void from_json(const json& j, Config& c){
	j.at("vhosts").get_to(c.vhosts);
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

