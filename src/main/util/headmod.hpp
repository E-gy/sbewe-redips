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
	inline HeadMod& rem(const std::string& h){
		remove.insert(h);
		return *this;
	}
	inline HeadMod& ren(const std::string& f, const std::string& t){
		rename[f] = t;
		return *this;
	}
	inline HeadMod& set(const std::string& h, const std::string& v){
		add[h] = v;
		return *this;
	}
};
