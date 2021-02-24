#pragma once

#include "http.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace redips::http {

struct RR {
	Version version;
	std::unordered_map<std::string, std::string> headers;
	std::optional<std::vector<char>> body; //TODO byte it up
	bool hasHeader(const std::string& h);
	bool hasHeader(Header h);
	std::optional<std::string> getHeader(const std::string& h);
	std::optional<std::string> getHeader(Header h);
	void setHeader(const std::string& h, std::string v);
	void setHeader(Header h, std::string v);
	void removeHeader(const std::string& h);
	void removeHeader(Header h);
};

struct Request : public RR {
	Method method;
	std::string path;
};

struct Response : public RR {
	Status status;
};

}
