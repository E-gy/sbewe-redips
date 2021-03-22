#pragma once

#include <string>

bool isValidIPv4(const std::string&);
bool isValidIPv6(const std::string&);
bool isValidIP(const std::string&);
inline bool isValidPort(int p){ return 0 <= p && p <= 0xFFFF; }
inline bool isValidPort(unsigned p){ return p <= 0xFFFF; }

struct IPp {
	std::string ip;
	unsigned port;
	inline std::string portstr() const noexcept { return std::to_string(port); }
	inline bool isValidIPv4() const noexcept { return ::isValidIPv4(ip); }
	inline bool isValidIPv6() const noexcept { return ::isValidIPv6(ip); }
	inline bool isValidIP() const noexcept { return ::isValidIP(ip); }
	inline bool isValidPort() const noexcept { return ::isValidPort(port); }
	inline bool operator==(const IPp& other) const noexcept { return ip == other.ip && port == other.port; }
};

std::string ipaddr2str(int af, const void* addr);

namespace std {
	template<> struct hash<IPp> {
		size_t operator()(const IPp& ip) const noexcept { return std::hash<std::string>{}(ip.ip) ^ (ip.port << 1); }
	};
}
