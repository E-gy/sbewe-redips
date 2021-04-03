#pragma once

#include "loadbal.hpp"
#include <fiz/helth.hpp>

namespace redips::virt {

template<typename T> struct HealThy {
	T t;
	fiz::HealthMonitor::SAH helth;
};

template<typename T> class FailOverBalancer {
	fiz::HealthMonitor* helthmon;
	std::vector<T> ts;
	std::vector<fiz::HealthMonitor::SAH> helth;
	bool lenient;
	public:
		FailOverBalancer(fiz::HealthMonitor* hmon, bool l = false) : helthmon(hmon), lenient(l) {}
		result<void, std::string> add(T && t, const IPp& haddr, const std::string& hpa){
			auto sah = helthmon->add(haddr, hpa);
			if(auto err = sah.err()) return *err;
			helth.push_back(std::move(*sah.ok()));
			ts.push_back(std::move(t));
			return result<void, std::string>::Ok();
		}
		std::optional<HealThy<T>> operator()(){
			for(unsigned i = 0; i < helth.size(); i++) switch(*helth[i]){
				case fiz::Health::Missing:
					if(lenient) [[fallthrough]];
					else break;
				case fiz::Health::Alive:
					return HealThy<T>{ts[i],helth[i]};
				case fiz::Health::Dead:
				default:
					break;
			}
			return std::nullopt;
		}
};

template<typename T> class FailRobinBalancer {
	fiz::HealthMonitor* helthmon;
	std::vector<T> ts;
	std::vector<fiz::HealthMonitor::SAH> helth;
	bool lenient;
	std::atomic_uint64_t next = 0;
	public:
		FailRobinBalancer(fiz::HealthMonitor* hmon, bool l = false) : helthmon(hmon), lenient(l) {}
		result<void, std::string> add(const T& t, const IPp& haddr, const std::string& hpa, unsigned w = 1){
			auto sah = helthmon->add(haddr, hpa);
			if(auto err = sah.err()) return *err;
			for(unsigned i = 0; i < w; i++){
				helth.push_back(*sah.ok());
				ts.push_back(t);
			}
			return result<void, std::string>::Ok();
		}
		std::optional<HealThy<T>> operator()(){
			for(unsigned k = 0; k < helth.size(); k++){
				unsigned i = next++%ts.size();
				switch(*helth[i]){
					case fiz::Health::Missing:
						if(lenient) [[fallthrough]];
						else break;
					case fiz::Health::Alive:
						return HealThy<T>{ts[i],helth[i]};
					case fiz::Health::Dead:
					default:
						break;
				}
			}
			return std::nullopt;
		}
};

}
