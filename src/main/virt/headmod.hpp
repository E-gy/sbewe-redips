#pragma once

#include <string>
#include <vector>
#include <util/headmod.hpp>

#include "vservr.hpp"

namespace redips::virt {

void headMod(http::RR& rr, const HeadMod& mod);

SServer modifyHeaders2(const HeadMod& fwd, const HeadMod& bwd, SServer);

}
