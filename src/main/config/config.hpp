#pragma once

#include <string>
#include <optional>
#include <vector>
#include <iostream>
#include <util/result.hpp>

namespace redips::config {

struct VHost {
	std::string ip;
	unsigned port;
	std::string serverName;
	std::string root;
	std::optional<std::string> defaultFile;
	VHost() = default;
};

struct Config {
	std::vector<VHost> vhosts;
};

result<Config, std::string> parse(std::istream&);
result<void, std::string> serialize(std::ostream&, const Config&);

}
