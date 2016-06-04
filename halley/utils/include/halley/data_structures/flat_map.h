#pragma once

#include <boost/container/flat_map.hpp>

namespace Halley {
	template<typename Key, typename T> using FlatMap = boost::container::flat_map<Key, T, std::less<Key>, std::allocator<std::pair<Key, T>>>;
}
