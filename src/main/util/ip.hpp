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
	inline bool operator==(const IPp& other){ return ip == other.ip && port == other.port; }
};

namespace std {
	template<> struct hash<IPp> {
		size_t operator()(const IPp& ip){ return std::hash<std::string>{}(ip.ip) ^ (ip.port << 1); }
	};
}
