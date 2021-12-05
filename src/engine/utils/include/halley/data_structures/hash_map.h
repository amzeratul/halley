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
	
	template<typename K, typename V> using HashMap = ska::flat_hash_map<K, V>;
}
