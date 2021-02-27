#pragma once

#include <string>
#include <unordered_map>

#include "vservr.hpp"

namespace redips::virt {

struct R1otBuilder {
	std::unordered_map<std::string, SServer> services;
	R1otBuilder() = default;
	/**
	 * Adds a service with specified name
	 * @returns `this`
	 */
	R1otBuilder& addService(const std::string& name, SServer service);
	/**
	 * Finalizes construction of the r1oter. Useless afterwards.
	 */
	SServer build();
};

}
