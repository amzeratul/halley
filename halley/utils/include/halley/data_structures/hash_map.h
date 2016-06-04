#pragma once

#include <unordered_map>

namespace Halley {
	template<typename Key, typename T> using HashMap = std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>, std::allocator<std::pair<const Key, T>>>;
}
