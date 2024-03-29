#pragma once

#include <yasync.hpp>
#include <yasync/ticktack.hpp>
#include <util/ip.hpp>
#include <ostream>
#include "estconn.hpp"

namespace redips::fiz {

enum class Health {
	/// Everything is awesome!
	Alive,
	/// The endpoint may simply be experiencing signignificant load, or may have been shut down unexpectedly. Tune in later to find out!
	Missing,
	/// Ded from all the _dad_ jokes you sent its way
	Dead,
};
inline auto healthToStr(Health h){
	switch(h){
		case Health::Alive: return "Alive";
		case Health::Missing: return "Missing";
		case Health::Dead: return "Dead";
		default: return "Wubba Lubba Dub Dub!";
	}
}
inline auto& operator<<(std::ostream& os, Health h){ return os << healthToStr(h); }

/**
 * Health status monitor.
 * A health check is performed regularly.
 * A health may complete early indicating resp. health (DoA) status.
 * If the check takes longer than 500ms, the status is declared missing.
 * If the check takes longer than 1/10th of the check interval, it is cancelled and status is declared dead.
 */
class HealthMonitor {
	yasync::io::IOYengine* engine;
	yasync::TickTack ticktack;
	const yasync::TickTack::Duration interval;
	public:
		HealthMonitor(yasync::io::IOYengine*, yasync::TickTack::Duration);
		void shutdown();
		using SAH = std::shared_ptr<std::atomic<Health>>;
		static constexpr auto MISSINGT = std::chrono::milliseconds(500);
		result<SAH, std::string> add(const IPp&, const std::string&);
	private:
		SAH _add(ConnectionFactory &&, const std::string&);
	// 	struct ISAH {
	// 		IPp addr;
	// 		std::string path;
	// 		SAH helth;
	// 		yasync::TickTack::Id timer;
	// 	};
	// 	std::vector<ISAH> sahs;
};


}
