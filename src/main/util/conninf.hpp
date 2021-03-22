#pragma once

#include <yasync/io.hpp>
#include <ostream>

struct ConnectionInfo {
	yasync::io::IOResource connection;
	std::string address;
	std::string protocol;
	std::optional<std::string> host;
	std::string to_string() const;
};
std::ostream& operator<<(std::ostream&, const ConnectionInfo&);
