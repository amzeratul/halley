#pragma once

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "../../../../../contrib/skarupke/flat_hash_map.hpp"
#include <string_view>

namespace Halley {

	class String;

	template <typename T>
	struct EqualToPicker {
		using type = std::equal_to<T>;
	};
	
	template <>
	struct EqualToPicker<String> {
		using type = std::equal_to<std::string_view>;
	};

	template<typename Key, typename Value, typename Hash = std::hash<Key>>
	using HashMap = ska::flat_hash_map<Key, Value, Hash, typename EqualToPicker<Key>::type>;

	template<typename Key, typename Hash = std::hash<Key>>
	using HashSet = ska::flat_hash_set<Key, Hash, typename EqualToPicker<Key>::type>;
}
