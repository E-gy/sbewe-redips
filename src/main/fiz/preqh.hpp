#pragma once

#include <virt/vservr.hpp>

namespace redips::fiz {

class ConnectionCare {
	public:
		/**
		 * Take care of connection pure HTTP exchange connection, after protocol specific layering, and until closure.
		 */
		void takeCare(yasync::io::IOResource, virt::SServer);
};

}
