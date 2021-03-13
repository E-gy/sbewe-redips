#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <util/hostresolv.hpp>

#include "vservr.hpp"

namespace redips::virt {

struct R1otBuilder {
	HostMapper<SServer> services;
	std::optional<SServer> defolt;
	R1otBuilder() = default;
	/**
	 * Adds a service with specified keying
	 * @returns `this`
	 */
	R1otBuilder& addService(const HostK& key, SServer service);
	/**
	 * Sets given service as default
	 */
	R1otBuilder& setDefaultService(SServer);
	/**
	 * Finalizes construction of the r1oter. Useless afterwards.
	 */
	SServer build();
};

}
