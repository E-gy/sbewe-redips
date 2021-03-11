#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include "vservr.hpp"

namespace redips::virt {

struct R1otBuilder {
	std::unordered_map<std::string, SServer> services;
	std::unordered_map<std::string, SServer> services2;
	std::optional<SServer> defolt;
	R1otBuilder() = default;
	/**
	 * Adds a service with specified name
	 * @returns `this`
	 */
	R1otBuilder& addService(const std::string& name, SServer service);
	R1otBuilder& addService2(const std::string&, SServer);
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
