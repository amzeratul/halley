#pragma once

/*
#include <EASTL/vector_map.h>

namespace Halley {
	template<typename Key, typename T> using FlatMap = eastl::vector_map<Key, T>;
}
*/

/*
#ifndef _LIBCPP_DISABLE_DEPRECATION_WARNINGS
#define _LIBCPP_DISABLE_DEPRECATION_WARNINGS
#endif
#include <boost/container/flat_map.hpp>

namespace Halley
{
	template<typename Key, typename T> using FlatMap = boost::container::flat_map<Key, T, std::less<Key>, std::allocator<std::pair<Key, T>>>;
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
	template<typename K, typename V> using FlatMap = ska::flat_hash_map<K, V>;
}

