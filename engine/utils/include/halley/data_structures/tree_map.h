#pragma once

#if HAS_EASTL

#include <EASTL/map.h>

namespace Halley {
	template<typename Key, typename T> using TreeMap = eastl::map<Key, T>;
}

#else

#include <map>

namespace Halley {
	template<typename Key, typename T> using TreeMap = std::map<Key, T>;
}

#endif
