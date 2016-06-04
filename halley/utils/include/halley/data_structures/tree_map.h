#pragma once

#include <map>

namespace Halley {
	template<typename Key, typename T> using TreeMap = std::map<Key, T, std::less<Key>, std::allocator<std::pair<const Key, T>>>;
}
