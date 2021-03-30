#pragma once

#include <string>
#include <optional>
#include <variant>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <util/result.hpp>
#include <util/hostresolv.hpp>
#include <util/ip.hpp>
#include <util/headmod.hpp>
#include <util/lbm.hpp>

namespace redips::config {

struct Timeout {
	unsigned seconds;
	inline operator unsigned() const { return seconds; }
};

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
	/// Generate index file for directories where default file does not exist
	std::optional<bool> autoindex;
};
struct Proxy {
	/// configured or anonynous upstream
	std::variant<std::string, IPp> upstream;
	/// Forwarded request modifier
	HeadMod fwdMod;
	/// Backwarded response modifier
	HeadMod bwdMod;
	/// Timeout for proxied transaction
	std::optional<Timeout> transactionTimeout;
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

using LoadBalancingMethod = ::LoadBalancingMethod;

struct Uphost {
	IPp address;
	std::optional<unsigned> weight;
	std::optional<std::string> healthCheckUrl;
};

struct Upstream {
	LoadBalancingMethod lbm;
	std::optional<unsigned> retries;
	std::vector<Uphost> hosts;
};

struct ThroughputUnlimiter {
	/// bytes
	unsigned b;
	/// time (seconds, fractional)
	double t;
};

struct Timings {
	std::optional<Timeout> keepAlive;
	std::optional<Timeout> transaction;
	std::optional<ThroughputUnlimiter> throughput;
};

struct Config {
	std::vector<VHost> vhosts;
	std::unordered_map<std::string, Upstream> upstreams;
	Timings timings;
};

result<Config, std::string> parse(std::istream&);
result<void, std::string> serialize(std::ostream&, const Config&);

}
