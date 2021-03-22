#pragma once

#include <string>
#include <optional>
#include <variant>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <util/result.hpp>
#include <util/hostresolv.hpp>
#include <util/ip.hpp>

namespace redips::config {

struct TLS {
	/// TLS certificate
	std::string cert;
	/// TLS pk
	std::string key;
};

struct BasicAuth {
	std::string realm;
	std::vector<std::string> credentials;
};

struct FileServer {
	/// Root location of file discovery
	std::string root;
	/// Default file to serve in the directory if the requested is directory
	std::optional<std::string> defaultFile;
};
struct Proxy {
	/// configured or anonynous upstream
	std::variant<std::string, IPp> upstream;
	struct HeadMod {
		/// Headers and respective values to remove
		std::unordered_set<std::string> remove;
		/// Headers to rename (from → to)
		std::unordered_map<std::string, std::string> rename;
		/// Headers and respective values to add
		std::unordered_map<std::string, std::string> add;
	};
	/// Forwarded request modifier
	HeadMod fwdMod;
	/// Backwarded response modifier
	HeadMod bwdMod;
};

struct VHost {
	/// Listening address
	IPp address;
	std::string serverName;
	std::variant<FileServer, Proxy> mode;
	std::optional<TLS> tls;
	std::optional<BasicAuth> auth;
	bool isDefault = false;
	VHost() = default;
	std::string identk();
	HostK tok();
};

enum class LoadBalancingMethod {
	RoundRobin, FailOver, FailRobin
};

struct Uphost {
	IPp address;
	std::optional<unsigned> weight;
	std::optional<std::string> healthCheckUrl;
};

struct Upstream {
	LoadBalancingMethod lbm;
	std::vector<Uphost> hosts;
};

struct Config {
	std::vector<VHost> vhosts;
	std::unordered_map<std::string, Upstream> upstreams;
};

result<Config, std::string> parse(std::istream&);
result<void, std::string> serialize(std::ostream&, const Config&);

}
