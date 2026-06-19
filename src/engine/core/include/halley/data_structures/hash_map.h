#pragma once

#include "../../../../../contrib/skarupke/flat_hash_map.hpp"
#include "halley/utils/hash.h"
#include <string_view>

#include "temp_allocator.h"

namespace Halley {

	template <typename T, typename Enable = void>
	struct hash {
		constexpr uint64_t operator()(const T& v) const noexcept
		{
			return std::hash<T>()(v);
		}
	};

	template <typename T>
	struct hash<T, std::enable_if_t<std::is_trivially_copyable_v<T>>> {
		constexpr uint64_t operator()(const T& v) const noexcept
		{
			return Hash::hash(v);
		}
	};

	template <>
	struct hash<std::string_view> {
		constexpr uint64_t operator()(const std::string_view& s) const noexcept
		{
			return Hash::hash(s.data(), s.length());
		}

		constexpr uint64_t operator()(const std::string& s) const noexcept
		{
			return Hash::hash(s.c_str(), s.length());
		}

		constexpr uint64_t operator()(const String& s) const noexcept
		{
			return Hash::hash(s.c_str(), s.length());
		}

		template<int N>
		constexpr uint64_t operator()(const char (&s)[N]) const noexcept
		{
			return Hash::hash(s, N);
		}
	};

	template <>
	struct hash<std::string> {
		constexpr uint64_t operator()(const std::string_view& s) const noexcept
		{
			return Hash::hash(s.data(), s.length());
		}

		constexpr uint64_t operator()(const std::string& s) const noexcept
		{
			return Hash::hash(s.c_str(), s.length());
		}

		constexpr uint64_t operator()(const String& s) const noexcept
		{
			return Hash::hash(s.c_str(), s.length());
		}

		template<int N>
		constexpr uint64_t operator()(const char (&s)[N]) const noexcept
		{
			return Hash::hash(s, N);
		}
	};
	
	template <>
	struct hash<const char*> {
		constexpr uint64_t operator()(const std::string_view& s) const noexcept
		{
			return Hash::hash(s.data(), s.length());
		}

		constexpr uint64_t operator()(const std::string& s) const noexcept
		{
			return Hash::hash(s.c_str(), s.length());
		}

		constexpr uint64_t operator()(const String& s) const noexcept
		{
			return Hash::hash(s.c_str(), s.length());
		}

		template<int N>
		constexpr uint64_t operator()(const char (&s)[N]) const noexcept
		{
			return Hash::hash(s, N);
		}
	};

	template <>
	struct hash<String> {
		constexpr uint64_t operator()(const std::string_view& s) const noexcept
		{
			return Hash::hash(s.data(), s.length());
		}

		constexpr uint64_t operator()(const std::string& s) const noexcept
		{
			return Hash::hash(s.c_str(), s.length());
		}

		constexpr uint64_t operator()(const String& s) const noexcept
		{
			return Hash::hash(s.c_str(), s.length());
		}

		template<int N>
		constexpr uint64_t operator()(const char (&s)[N]) const noexcept
		{
			return Hash::hash(s, N);
		}
	};


	class String;

	template <typename T>
	struct EqualToPicker {
		using type = std::equal_to<T>;
	};
	
	template <>
	struct EqualToPicker<String> {
		using type = std::equal_to<std::string_view>;
	};

	template <typename T>
	using DefaultHash = std::hash<T>;

	template<typename Key, typename Value, typename Hash = DefaultHash<Key>, typename Allocator = std::allocator<std::pair<Key, Value>>>
	using HashMap = ska::flat_hash_map<Key, Value, Hash, typename EqualToPicker<Key>::type, Allocator>;

	template<typename Key, typename Hash = DefaultHash<Key>, typename Allocator = std::allocator<Key>>
	using HashSet = ska::flat_hash_set<Key, Hash, typename EqualToPicker<Key>::type, Allocator>;
	
	template<typename Key, typename Value, typename Hash = DefaultHash<Key>>
	using HashMapTemp = ska::flat_hash_map<Key, Value, Hash, typename EqualToPicker<Key>::type, TempPoolAllocator<std::pair<Key, Value>>>;

	template<typename Key, typename Hash = DefaultHash<Key>>
	using HashSetTemp = ska::flat_hash_set<Key, Hash, typename EqualToPicker<Key>::type, TempPoolAllocator<Key>>;

}

namespace std {
	template<typename A, typename B>
	struct hash<std::pair<A, B>> {
        std::size_t operator()(const std::pair<A, B>& v) const noexcept
        {
			return Halley::Hash::combineHash(Halley::DefaultHash<A>()(v.first),Halley:: DefaultHash<B>()(v.second));
        }
    };

	template<typename A, typename B, typename C>
	struct hash<std::tuple<A, B, C>> {
        std::size_t operator()(const std::tuple<A, B, C>& v) const noexcept
        {
			return Halley::Hash::combineHash(Halley::Hash::combineHash(Halley::DefaultHash<A>()(std::get<0>(v)),Halley:: DefaultHash<B>()(std::get<1>(v))), Halley::DefaultHash<C>()(std::get<2>(v)));
        }
    };
}
