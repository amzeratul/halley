#pragma once
#include <typeinfo>
#include "halley/support/assert.h"
#include <gsl/span>
#include <sstream>
#include <algorithm>
#include "halleystring.h"
#include <halley/support/exception.h>
#include <array>
#include <optional>
#include <iomanip>
#include "halley/data_structures/maybe.h"
#include "enum_names.h"
#include "halley/support/logger.h"
#include <charconv>
#include <cstring>

#include "halley/game/game_platform.h"

namespace Halley
{
	inline String toString(bool value)
	{
		return value ? "true" : "false";
	}

	template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
	String toString(T src, int precisionDigits = -1, char decimalSeparator = '.', bool fixed = true)
	{
		HalleyAssertDev(precisionDigits >= -1 && precisionDigits <= 20);

		constexpr int bufSize = 32;
		std::array<char, bufSize> buffer;

		const std::chars_format format = fixed ? std::chars_format::fixed : std::chars_format::general;
		const std::to_chars_result result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), src, format, precisionDigits);
		if (result.ec != std::errc()) [[unlikely]] {
			return "";
		}

		// Replace decimal separator, if needed
		if (decimalSeparator != '.') [[unlikely]] {
			for (char* c = buffer.data(); c != result.ptr; ++c) {
				if (*c == '.') {
					*c = decimalSeparator;
				}
			}
		}

		// Generate string
		auto str = std::string_view(buffer.data(), result.ptr - buffer.data());

		// Apply pretty float algorithm
		if (precisionDigits == -1) [[likely]] {
			str = String::prettyFloat(str, decimalSeparator);
		}

		return str;
	}

	template <typename T, typename std::enable_if<std::is_integral_v<T> && !std::is_same_v<T, bool>, int>::type = 0>
	String toString(T value, int base = 10, int width = 1, char fill = '0', char thousandsSeparator = 0)
	{
		constexpr int maxWidth = 32;
		constexpr int bufSize = 32;

		HalleyAssertDebug(base >= 2 && base <= 36);
		HalleyAssertDebug(width <= maxWidth);

		std::array<char, bufSize + maxWidth> buffer;
		const std::to_chars_result result = std::to_chars(buffer.data() + maxWidth, buffer.data() + buffer.size(), value, base);
		if (result.ec != std::errc()) [[unlikely]] {
			return "";
		}

		char* startPos = buffer.data() + maxWidth;
		char* endPos = result.ptr;

		const size_t len = endPos - startPos;
		if (static_cast<int>(len) < width) {
			const auto diff = width - static_cast<int>(len);
			HalleyAssertDebug(diff <= maxWidth);
			memset(startPos - diff, fill, diff);
			startPos -= diff;
		}

		const auto str = std::string_view(startPos, endPos - startPos);

		if (thousandsSeparator != 0) [[unlikely]] {
			return String::integerAddThousandsSeparator(str, thousandsSeparator);
		} else {
			return str;
		}
	}

	struct UserConverter
	{
		template<typename T>
		static String toString(const T& v)
		{
			if constexpr (std::is_enum<T>::value) {
				return EnumNames<T>()().at(int(v));
			} else if constexpr (std::is_same_v<T, String>) {
				return v;
			} else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
				return Halley::toString(v);
			} else {
				return v.toString();
			}
		}

		template<typename T>
		static T fromString(const String& str)
		{
			if constexpr (std::is_enum_v<T>) {
				EnumNames<T> n;
				auto names = n();
				auto res = std::find_if(std::begin(names), std::end(names), [&](const char* v) { return str == v; });
				if (res == std::end(names)) {
					Logger::logError("String \"" + str + "\" does not exist in enum \"" + typeid(T).name() + "\".");
					return {};
				}
				return T(res - std::begin(names));
			} else if constexpr (std::is_integral_v<T>) {
				return str.toInteger();
			} else if constexpr (std::is_floating_point_v<T>) {
				return str.toFloat();
			} else {
				return T(str);
			}
		}

		template<typename T>
		static std::optional<T> tryFromString(const String& str)
		{
			if constexpr (std::is_enum_v<T>) {
				EnumNames<T> n;
				auto names = n();
				auto res = std::find_if(std::begin(names), std::end(names), [&](const char* v) { return str == v; });
				if (res == std::end(names)) {
					return std::nullopt;
				}
				return T(res - std::begin(names));
			} else if constexpr (std::is_integral_v<T>) {
				return str.isInteger() ? std::make_optional(str.toInteger()) : std::nullopt;
			} else if constexpr (std::is_floating_point_v<T>) {
				return str.isNumber() ? std::make_optional(str.toFloat()) : std::nullopt;
			} else {
				return T(str);
			}
		}
	};


	template <typename T>
	struct ToStringConverter {
		String operator()(const T& s) const
		{
			return UserConverter::toString(s);
		}
	};

	template <typename T>
	struct FromStringConverter {
		T operator()(const String& s) const
		{
			return UserConverter::fromString<T>(s);
		}
	};

	template <typename T>
	struct TryFromStringConverter {
		std::optional<T> operator()(const String& s) const
		{
			return UserConverter::tryFromString<T>(s);
		}
	};

	template<size_t N>
	struct ToStringConverter<char[N]>
	{
		String operator()(const char s[N]) const
		{
			return String(s);
		}
	};
	
	template<>
	struct ToStringConverter<const char*>
	{
		String operator()(const char* s) const
		{
			return String(s);
		}
	};

	template<>
	struct ToStringConverter<char*>
	{
		String operator()(const char* s) const
		{
			return String(s);
		}
	};

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
	template<size_t N>
	struct ToStringConverter<char8_t[N]>
	{
		String operator()(const char8_t s[N]) const
		{
			return String(s);
		}
	};

	template<>
	struct ToStringConverter<const char8_t*>
	{
		String operator()(const char8_t* s) const
		{
			return String(s);
		}
	};

	template<>
	struct ToStringConverter<char8_t*>
	{
		String operator()(char8_t* s) const
		{
			return String(s);
		}
	};
#endif

	template<size_t N>
	struct ToStringConverter<wchar_t[N]>
	{
		String operator()(const wchar_t s[N]) const
		{
			return String(s);
		}
	};

	template<>
	struct ToStringConverter<const wchar_t*>
	{
		String operator()(const wchar_t* s) const
		{
			return String(s);
		}
	};

	template<>
	struct ToStringConverter<wchar_t*>
	{
		String operator()(const wchar_t* s) const
		{
			return String(s);
		}
	};

	template<>
	struct ToStringConverter<std::string>
	{
		String operator()(const std::string& s) const
		{
			return String(s);
		}
	};

	template<>
	struct ToStringConverter<bool>
	{
		String operator()(bool s) const
		{
			return String(s ? "true" : "false");
		}
	};

	template<>
	struct FromStringConverter<bool>
	{
		bool operator()(const String& s) const
		{
			return s == "true";
		}
	};

	template <typename T, typename std::enable_if<!(std::is_integral_v<T> && !std::is_same_v<T, bool>) && !std::is_floating_point_v<T>, int>::type = 0>
	String toString(const T& value)
	{
		return ToStringConverter<typename std::remove_cv<T>::type>()(value);
	}



	template <typename T>
	T fromString(const String& value)
	{
		return FromStringConverter<T>()(value);
	}

	template <typename T>
	std::optional<T> tryFromString(const String& value)
	{
		return TryFromStringConverter<T>()(value);
	}



	template <typename T, typename std::enable_if<!std::is_same<T, String>::value, int>::type = 0>
	String operator+ (const String& lhp, const T& rhp)
	{
		return lhp + toString<typename std::remove_cv<T>::type>(rhp);
	}

	template <typename T, typename std::enable_if<!std::is_same<T, String>::value, int>::type = 0>
	String operator+ (const T& lhp, const String& rhp)
	{
		return toString<typename std::remove_cv<T>::type>(lhp) + rhp;
	}


	template <typename T>
	String toString(gsl::span<const T> values, std::string_view separator)
	{
		std::stringstream ss;
		ss << "[";
		for (size_t i = 0; i < values.size(); i++) {
			if (i != 0) {
				ss << separator;
			}
			ss << toString(values[i]).cppStr();
		}
		ss << "]";
		return ss.str();
	}

	template <typename T, typename F>
	String toString(gsl::span<const T> values, std::string_view separator, F f)
	{
		std::stringstream ss;
		ss << "[";
		for (size_t i = 0; i < values.size(); i++) {
			if (i != 0) {
				ss << separator;
			}
			ss << f(values[i]).cppStr();
		}
		ss << "]";
		return ss.str();
	}
	
	template <typename T>
	String toString(const Vector<T>& values, std::string_view separator)
	{
		return toString(gsl::span<const T>(values), separator);
	}
	
	template <typename T, typename F>
	String toString(const Vector<T>& values, std::string_view separator, F f)
	{
		return toString(gsl::span<const T>(values), separator, std::move(f));
	}

	template<typename T, typename U>
	struct ToStringConverter<std::pair<T, U>>
	{
		String operator()(const std::pair<T, U>& v) const
		{
			return "{" + toString(v.first) + ", " + toString(v.second) + "}";
		}
	};

	template<typename T>
	struct ToStringConverter<std::optional<T>>
	{
		String operator()(const std::optional<T>& v) const
		{
			if (v) {
				return toString(v.value());
			} else {
				return "{}";
			}
		}
	};

	template<typename T>
	struct ToStringConverter<OptionalLite<T>>
	{
		String operator()(const OptionalLite<T>& v) const
		{
			if (v) {
				return toString(v.value());
			} else {
				return "{}";
			}
		}
	};

	template<typename T>
	struct ToStringConverter<Vector<T>>
	{
		String operator()(const Vector<T>& v) const
		{
			String result;
			for (size_t i = 0; i < v.size(); i++) {
				if (i != 0) {
					result += ", ";
				}
				result += toString(v[i]);
			}
			return result;
		}
	};

	template<typename T>
	struct ToStringConverter<T*>
	{
		String operator()(const T* ptr) const
		{
			if constexpr (sizeof(T*) == sizeof(uint64_t)) {
				return "0x" + toString(reinterpret_cast<uint64_t>(ptr), 16, sizeof(T*) * 2);
			} else {
				return "0x" + toString(reinterpret_cast<uint32_t>(ptr), 16, sizeof(T*) * 2);
			}
		}
	};

	inline std::optional<int> stringViewToInt(const std::string_view& input)
	{
	    int out;
	    const auto result = std::from_chars(input.data(), input.data() + input.size(), out);
	    if (result.ec == std::errc::invalid_argument || result.ec == std::errc::result_out_of_range) {
	        return std::nullopt;
	    }
	    return out;
	}

	inline std::optional<float> stringViewToFloat(const std::string_view& input)
	{
		std::array<char, 16> buffer;
		buffer.fill(0);
		std::memcpy(buffer.data(), input.data(), std::min(input.size(), buffer.size() - 1));
		return static_cast<float>(std::atof(buffer.data()));
	}

	inline std::optional<double> stringViewToDouble(const std::string_view& input)
	{
		std::array<char, 16> buffer;
		buffer.fill(0);
		std::memcpy(buffer.data(), input.data(), std::min(input.size(), buffer.size() - 1));
		return std::atof(buffer.data());
	}

	template <typename T>
	String String::concat(gsl::span<const T> list, std::string_view separator)
	{
		Vector<String> text;
		text.reserve(list.size());
		for (const auto& l: list) {
			text.push_back(Halley::toString(l));
		}
		return concatList(text, separator);
	}
}
