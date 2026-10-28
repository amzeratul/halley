#pragma once

#include <string_view>
#include "halley/utils/hash.h"
#include "halley/text/halleystring.h"

namespace Halley {
    class PrehashedStringView {
    public:
        constexpr PrehashedStringView(std::string_view str)
            : str(str)
            , hash(Hash::hash(str.data(), str.size()))
        {
        }

        constexpr PrehashedStringView(const String& str)
            : str(str)
            , hash(Hash::hash(str.c_str(), str.length()))
        {
        }

        constexpr PrehashedStringView(const std::string& str)
            : str(str)
            , hash(Hash::hash(str.c_str(), str.length()))
        {
        }

        template <size_t N>
        consteval PrehashedStringView(const char (&str)[N])
            : str(str, N - 1)
            , hash(Hash::hash(str, N - 1))
        {
        }

        template <typename T>
			requires std::is_pointer_v<std::remove_cvref_t<T>> && std::is_convertible_v<T, const char*>
        constexpr PrehashedStringView(T&& ptr)
            : str(ptr)
            , hash(Hash::hash(str.data(), str.size()))
        {
        }

        constexpr bool operator==(const PrehashedStringView& other) const
        {
            return str == other.str;
        }
        
        constexpr bool operator==(const std::string_view& other) const
        {
            return str == other;
        }

        constexpr auto operator<=>(const PrehashedStringView& other) const
        {
            return str <=> other.str;
        }
        
        constexpr auto operator<=>(const std::string_view& other) const
        {
            return str <=> other;
        }

        std::string_view str;
        uint64_t hash;
    };
}

namespace std {
	template<>
	struct hash<Halley::PrehashedStringView>
	{
		using is_transparent = void;

		constexpr size_t operator()(const Halley::PrehashedStringView& s) const noexcept { return s.hash; }
	};
}
