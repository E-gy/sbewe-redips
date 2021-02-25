#include "ip.hpp"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

bool isValidIPv4(const std::string& str){
	char _[sizeof(::in_addr)];
	return ::inet_pton(AF_INET, str.c_str(), &_) == 1;
}

bool isValidIPv6(const std::string& str){
	char _[sizeof(::in6_addr)];
	return ::inet_pton(AF_INET, str.c_str(), &_) == 1;
}

bool isValidIP(const std::string& str){ return isValidIPv6(str) || isValidIPv4(str); }
