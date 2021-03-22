#pragma once

#include <unordered_set>
#include <unordered_map>

/**
 * Modifications are applied in following order:
 * 1. Remove
 * 2. Rename
 * 3. Add(/set)
 */
struct HeadMod {
	/// Headers and respective values to remove
	std::unordered_set<std::string> remove;
	/// Headers to rename (from → to)
	std::unordered_map<std::string, std::string> rename;
	/// Headers and respective values to add
	std::unordered_map<std::string, std::string> add;
	inline operator bool(){ return remove.size() || rename.size() || add.size(); }
};
