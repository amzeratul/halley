#pragma once

#if HAS_EASTL

#include <EASTL/vector_map.h>

namespace Halley {
	template<typename Key, typename T> using FlatMap = eastl::vector_map<Key, T>;
}

#else

#include <boost/container/flat_map.hpp>

namespace Halley
{
	template<typename Key, typename T> using FlatMap = boost::container::flat_map<Key, T, std::less<Key>, std::allocator<std::pair<Key, T>>>;
}

#endif
