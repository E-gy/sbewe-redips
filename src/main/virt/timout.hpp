#pragma once

#include <yasync/ticktack.hpp>
#include "vservr.hpp"

namespace redips::virt {

SServer timeItOut(yasync::Yengine*, yasync::TickTack*, const yasync::TickTack::Duration&, SServer);

}
