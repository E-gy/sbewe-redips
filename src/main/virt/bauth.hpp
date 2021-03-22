#pragma once

#include <string>
#include <vector>

#include "vservr.hpp"

namespace redips::virt {

SServer putBehindBasicAuth(const std::string& realm, const std::vector<std::string>& credentials, SServer, bool proxh = false);

}
