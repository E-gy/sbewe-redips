#pragma once

#include <string>
#include <optional>
#include <vector>
#include <iostream>
#include <util/result.hpp>
#include <util/hostresolv.hpp>

namespace redips::config {

struct SSL {
	std::string key, cert;
};

struct BasicAuth {
	std::string realm;
	std::vector<std::string> credentials;
};

struct VHost {
	std::string ip;
	unsigned port;
	std::string serverName;
	std::string root;
	std::optional<std::string> defaultFile;
	std::optional<SSL> ssl;
	std::optional<BasicAuth> auth;
	bool isDefault = false;
	VHost() = default;
	std::string identk();
	HostK tok();
};

struct Config {
	std::vector<VHost> vhosts;
};

result<Config, std::string> parse(std::istream&);
result<void, std::string> serialize(std::ostream&, const Config&);

}
