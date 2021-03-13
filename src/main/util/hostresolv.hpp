#pragma once

#include <unordered_map>
#include <string>

/**
 * Host keyeing
 */
struct HostK {
	std::string name;
	std::string addr;
	unsigned port;
};

template<typename T> class HostMapper {
	std::unordered_map<std::string, T> primary;
	std::unordered_map<std::string, T> secondary;
	public:
		HostMapper() = default;
		HostMapper& addHost(T host, const std::string& name, const std::string& addr, unsigned port){
			primary[name] = host;
			auto psuff = ":" + std::to_string(port);
			secondary[name + psuff] = host;
			secondary[addr] = host;
			secondary[addr + psuff] = host;
			return *this;
		}
		HostMapper& addHost(T host, const HostK& k){
			return addHost(host, k.name, k.port, k.addr);
		}
		std::optional<T> resolvePrimary(const std::string& q) const {
			return primary.count(q) > 0 ? primary[q] : std::nullopt;
		}
		std::optional<T> resolve(const std::string& q) const {
			if(auto prim = resolvePrimary(q)) return prim;
			if(secondary.count(q) > 0) return secondary[q];
			return std::nullopt;
		}
		std::optional<T> operator[](const std::string& q) const { return resolve(q); }
};
