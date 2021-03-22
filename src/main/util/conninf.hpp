#pragma once

#include <yasync/io.hpp>

struct ConnectionInfo {
	yasync::io::IOResource connection;
	std::string address;
	std::string protocol;
	std::optional<std::string> host;
};
