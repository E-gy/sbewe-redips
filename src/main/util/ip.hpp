#pragma once

#include <string>

bool isValidIPv4(const std::string&);
bool isValidIPv6(const std::string&);
bool isValidIP(const std::string&);
inline bool isValidPort(int p){ return 0 <= p && p <= 0xFFFF; }
inline bool isValidPort(unsigned p){ return p <= 0xFFFF; }
