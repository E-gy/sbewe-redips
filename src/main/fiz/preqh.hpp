#pragma once

#include <virt/vservr.hpp>
#include <unordered_set>

namespace redips::fiz {

class ConnectionCare {
	bool sdown = false;
	std::mutex idlok;
	std::unordered_set<yasync::io::IOResource> idle;
	void setIdle(yasync::io::IOResource);
	void unsetIdle(yasync::io::IOResource);
	public:
		/**
		 * Initiates care shut down.
		 * All idle `keep-alive` connections are closed.
		 * All further responses will be marked as `close` and resp closed.
		 * New connections are still accepted and processed, with respect to the above.
		 * The care MUST _NOT_ be nuked until IO `wioe` completes (eq destruction of the IO Yengine).
		 */
		void shutdown();
		/**
		 * Take care of connection pure HTTP exchange connection, after protocol specific layering, and until closure.
		 */
		void takeCare(yasync::io::IOResource, virt::SServer);
};

}
