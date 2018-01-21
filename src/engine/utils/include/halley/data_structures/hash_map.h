#pragma once

#include <unordered_map>

#if HAS_EASTL

#include <EASTL/hash_map.h>

namespace Halley {
	template<typename Key, typename T> using HashMap = eastl::hash_map<Key, T, std::hash<Key>>;
}

#else

namespace Halley {
	template<typename Key, typename T> using HashMap = std::unordered_map<Key, T, std::hash<Key>>;
}

#endif
