#pragma once

/*

#include <EASTL/hash_map.h>

namespace Halley {
	template<typename Key, typename T> using HashMap = eastl::hash_map<Key, T, std::hash<Key>>;
}

*/

/*
#include <unordered_map>

namespace Halley {
	template<typename Key, typename T> using HashMap = std::unordered_map<Key, T, std::hash<Key>>;
}
*/


#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "../../../../../contrib/skarupke/flat_hash_map.hpp"

namespace Halley {
	
	template<typename Key, typename Value, typename Hash = std::hash<Key>> using HashMap = ska::flat_hash_map<Key, Value, Hash>;
	template<typename T, typename Hash = std::hash<T>> using HashSet = ska::flat_hash_set<T, Hash>;
}
