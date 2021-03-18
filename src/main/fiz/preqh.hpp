#pragma once

#include <virt/vservr.hpp>

namespace redips::fiz {

/**
 * Take care of connection pure HTTP exchange connection, after protocol specific layering, and until closure.
 */
void takeCareOfConnection(yasync::io::IOResource, virt::SServer);

}
