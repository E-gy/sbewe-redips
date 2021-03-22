#pragma once

#include <vector>
#include <random>
#include <atomic>

namespace redips::virt {

/*
 * A balancer's job is to distribute <whatever> over underlying *t*hings.
 * It is effectively a no-args functor that returns a [next] thing on every call.
 * Obviously it is MT safe.
 */

template<typename T> class Unbalancer {
	T t;
	public:
		explicit Unbalancer(const T& tt) : t(tt) {}
		T operator()(){ return t; }
};

template<typename T> class RandomBalancer {
	std::vector<T> ts;
	public:
		RandomBalancer() = default;
		explicit RandomBalancer(const std::vector<T>& tts) : ts(tts) {}
		void add(const T& t, unsigned w = 1){ for(unsigned i = 0; i < w; i++) ts.push_back(t); }
		void add(T && t){ ts.push_back(std::move(t)); }
		T operator()(){
			static thread_local std::minstd_rand rnd;
			return ts[rnd()%ts.size()];
		}
};

template<typename T> class RoundRobinBalancer {
	std::vector<T> ts;
	std::atomic_uint64_t next = 0;
	public:
		RoundRobinBalancer() = default;
		explicit RoundRobinBalancer(const std::vector<T>& tts) : ts(tts) {}
		void add(const T& t, unsigned w = 1){ for(unsigned i = 0; i < w; i++) ts.push_back(t); }
		void add(T && t){ ts.push_back(std::move(t)); }
		T operator()(){
			return ts[next++%ts.size()];
		}
};

}
