/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/


#pragma once

#include <string>
#include <halley/data_structures/vector.h>
#include "halley/support/assert.h"
#include <gsl/span>
#include <cstdint>
#include <limits>

namespace Halley {

	typedef char Character;
	typedef wchar_t utf16type;
	typedef char32_t utf32type;
	typedef std::wstring StringUTF16;
	typedef std::u32string StringUTF32;

	template <typename T>
	class UnicodeViewIterator {
	public:
		constexpr UnicodeViewIterator(std::basic_string_view<T> view = {})
			: view(view)
		{
			next();
		}

#ifdef __cpp_lib_concepts
	    using iterator_concept = std::contiguous_iterator_tag;
#endif // __cpp_lib_concepts
	    using iterator_category = std::forward_iterator_tag;
	    using value_type        = char32_t;

		value_type operator*() const { return nextValue; }
		
		UnicodeViewIterator& operator++() { next(); return *this; }
		UnicodeViewIterator operator++(int) const { auto i = UnicodeViewIterator(this); i.next(); return i; }

		bool operator==(const UnicodeViewIterator& other) const { return view == other.view; }
		bool operator!=(const UnicodeViewIterator& other) const { return view != other.view; }

		friend void swap(UnicodeViewIterator& a, UnicodeViewIterator& b) noexcept { std::swap(a.view, b.view); }

	private:
		std::basic_string_view<T> view;
		char32_t nextValue;

		constexpr void next();
	};

	template <typename T>
	class UnicodeView {
	public:
		constexpr UnicodeView(std::basic_string_view<T> view)
			: view(view)
		{}

		[[nodiscard]] constexpr UnicodeViewIterator<T> begin() const
		{
			return UnicodeViewIterator<T>(view);
		}
		
		[[nodiscard]] constexpr UnicodeViewIterator<T> end() const
		{
			return UnicodeViewIterator<T>(std::basic_string_view<T>());
		}

	private:
		std::basic_string_view<T> view;
	};

	// String class
	class String {
	public:
		const static size_t npos = size_t(-1);

		String();
		String(const char* utf8);
		String(const char* utf8, size_t bytes);
		String(const char8_t* utf8);
		String(std::string_view strView);
		String(std::u8string_view strView);
		String(const std::basic_string<Character>& str);
		String(std::basic_string<Character>&& str);
		String(const String& str) noexcept;
		String(String&& str) noexcept;

		explicit String(const wchar_t* utf16);
		explicit String(std::wstring_view utf16);
		explicit String(std::u32string_view utf32);
		explicit String(const StringUTF32 &utf32);
		explicit String(char character);
		explicit String(wchar_t character);
		explicit String(char32_t utf32Character);
		explicit String(int character);
		explicit String(float number);
		explicit String(double number);
		explicit String(const Bytes& bytes);

		String& operator=(const char* utf8);
		String& operator=(std::basic_string<Character>&& str);
		String& operator=(const std::basic_string<Character>& str);
		String& operator=(String&& str) noexcept;
		String& operator=(const String& str);

		operator std::string() const;
		
		[[nodiscard]] constexpr bool isEmpty() const { return str.empty(); }
		[[nodiscard]] constexpr size_t length() const { return str.length(); }
		
		void setSize(size_t size);
		void truncate(size_t size);

		String& trim(bool fromRight);
		String& trimBoth();

		[[nodiscard]] bool contains(Character chr) const;
		[[nodiscard]] bool contains(std::string_view string, bool caseSensitive = true, bool paramIsPreLowercased = false) const;
		[[nodiscard]] bool contains(std::u8string_view string, bool caseSensitive = true, bool paramIsPreLowercased = false) const;
		[[nodiscard]] size_t find(std::string_view str, bool caseSensitive = true, bool paramIsPreLowercased = false) const;
		[[nodiscard]] size_t find(std::u8string_view str, bool caseSensitive = true, bool paramIsPreLowercased = false) const;

		[[nodiscard]] String replaceAll(std::string_view before, std::string_view after) const;
		[[nodiscard]] String replaceAll(std::u8string_view before, std::u8string_view after) const;
		[[nodiscard]] String replaceOne(std::string_view before, std::string_view after) const;
		[[nodiscard]] String replaceOne(std::u8string_view before, std::u8string_view after) const;
		void shrink();

		[[nodiscard]] String left(size_t n) const;
		[[nodiscard]] String right(size_t n) const;
		[[nodiscard]] String mid(size_t start, size_t count = npos) const;

		[[nodiscard]] bool startsWith(std::string_view string, bool caseSensitive = true) const;
		[[nodiscard]] bool startsWith(std::u8string_view string, bool caseSensitive = true) const;
		[[nodiscard]] bool startsWithAnyOf(gsl::span<const String> strings, bool caseSensitive = true) const;
		[[nodiscard]] bool endsWith(std::string_view string, bool caseSensitive = true) const;
		[[nodiscard]] bool endsWith(std::u8string_view string, bool caseSensitive = true) const;

		void writeText(const Character* src, size_t len, size_t &pos);
		void writeChar(const Character &src, size_t &pos);
		void writeNumber(Character *temp, int number, int pad, size_t &pos);

		[[nodiscard]] static bool isNumber(std::string_view str);
		[[nodiscard]] static bool isInteger(std::string_view str);
		[[nodiscard]] bool isNumber() const;
		[[nodiscard]] bool isInteger() const;
		[[nodiscard]] static bool isAlphanumeric(uint32_t character);

		[[nodiscard]] String asciiLower() const;
		[[nodiscard]] String asciiUpper() const;
		void asciiMakeUpper();
		void asciiMakeLower();
		bool asciiCompareNoCase(const Character *src) const;

		void appendCharacter(int unicode);

		// Convert a string to a number
		[[nodiscard]] int32_t toInteger() const;
		[[nodiscard]] int64_t toInteger64() const;
		[[nodiscard]] uint32_t toUInteger() const;
		[[nodiscard]] uint64_t toUInteger64() const;
		[[nodiscard]] float toFloat() const;
		[[nodiscard]] double toDouble() const;
		[[nodiscard]] int subToInteger(size_t start,size_t end) const;

		[[nodiscard]] static int32_t toInteger(std::string_view str);
		[[nodiscard]] static int64_t toInteger64(std::string_view str);
		[[nodiscard]] static uint32_t toUInteger(std::string_view str);
		[[nodiscard]] static uint64_t toUInteger64(std::string_view str);
		[[nodiscard]] static float toFloat(std::string_view str);
		[[nodiscard]] static double toDouble(std::string_view str);

		// std::string methods
		[[nodiscard]] const char* c_str() const;
		[[nodiscard]] String substr(size_t pos, size_t len=npos) const;
		[[nodiscard]] size_t find(Character character, size_t pos=0) const;
		[[nodiscard]] size_t find(const char* str, size_t pos=0) const;
		[[nodiscard]] size_t find_last_of(char character) const;
		[[nodiscard]] size_t size() const;
		[[nodiscard]] const char& operator[](size_t pos) const;
		[[nodiscard]] char& operator[](size_t pos);

		[[nodiscard]] Bytes toBytes() const;
		[[nodiscard]] gsl::span<const char> asSpan() const;
		[[nodiscard]] gsl::span<char> asSpan();
		[[nodiscard]] gsl::span<const std::byte> asByteSpan() const;
		[[nodiscard]] gsl::span<std::byte> asWriteableByteSpan();

		// Number tidy up functions
		[[nodiscard]] static String prettyFloat(String src, char decimalSeparator = '.');
		[[nodiscard]] static std::string_view prettyFloat(std::string_view src, char decimalSeparator = '.');
		[[nodiscard]] static String prettySize(uint64_t bytes);
		[[nodiscard]] static String integerAddThousandsSeparator(std::string_view str, char thousandsSeparator);

		// Unicode routines
		[[nodiscard]] StringUTF16 getUTF16() const;
		[[nodiscard]] StringUTF32 getUTF32() const;
		[[nodiscard]] size_t getUTF32Len() const;

		// Static unicode routines
		[[nodiscard]] static size_t getUTF8Len(std::string_view utf8);
		[[nodiscard]] static size_t getUTF8Len(std::u16string_view utf16);
		[[nodiscard]] static size_t getUTF8Len(std::wstring_view utf16);
		[[nodiscard]] static size_t getUTF8Len(std::u32string_view utf32);
		[[nodiscard]] static size_t getUTF16Len(const StringUTF32 &utf32);
		[[nodiscard]] static size_t getUTF32Len(std::string_view str);
		[[nodiscard]] static size_t getUTF32Len(std::u32string_view str);
		[[nodiscard]] static std::pair<char32_t, int> extractNextCharacter(std::string_view str);
		[[nodiscard]] static std::pair<char32_t, int> extractNextCharacter(std::u32string_view str);
		[[nodiscard]] static std::optional<char32_t> extractNextCharacterAndAdvance(std::string_view& str);
		[[nodiscard]] static std::optional<char32_t> extractNextCharacterAndAdvance(std::u32string_view& str);
		[[nodiscard]] UnicodeView<char> getUnicodeView() const;

		[[nodiscard]] inline std::string& cppStr() { return str; }
		[[nodiscard]] inline const std::string& cppStr() const { return str; }

		[[nodiscard]] Vector<String> split(char delimiter, size_t limit = std::numeric_limits<size_t>::max()) const;
		[[nodiscard]] Vector<String> split(std::string_view delimiter, size_t limit = std::numeric_limits<size_t>::max()) const;

		[[nodiscard]] static std::pair<std::string_view, std::string_view> split(std::string_view src, char delimeter);
		[[nodiscard]] static std::string_view splitAndAdvance(std::string_view& src, char delimeter);

		[[nodiscard]] static String concatList(gsl::span<const String> list, std::string_view separator);

		template <typename T>
		[[nodiscard]] static String concat(gsl::span<const T> list, std::string_view separator);

		template <typename T, typename F>
		[[nodiscard]] static String concat(gsl::span<const T> list, std::string_view separator, F toStringConv)
		{
			Vector<String> text;
			text.reserve(list.size());
			for (const auto& l: list) {
				text.push_back(toStringConv(l));
			}
			return concatList(text, separator);
		}

		static gsl::span<char> appendToBuffer(gsl::span<char> buffer, const char* str)
		{
			const size_t n = std::min(strlen(str), buffer.size());
			memcpy(buffer.data(), str, n);
			return buffer.subspan(n);
		}

		static gsl::span<char> appendToBuffer(gsl::span<char> buffer, std::string_view str)
		{
			const size_t n = std::min(str.size(), buffer.size());
			memcpy(buffer.data(), str.data(), n);
			return buffer.subspan(n);
		}

		static gsl::span<char> appendToBuffer(gsl::span<char> buffer, const String& str)
		{
			const size_t n = std::min(str.length(), buffer.size());
			memcpy(buffer.data(), str.c_str(), n);
			return buffer.subspan(n);
		}

		template <typename ... Ts>
		[[nodiscard]] static std::string_view concatInBuffer(gsl::span<char> buffer, const Ts&... params)
		{
			auto b = buffer;
			([&] {
				static_assert(!std::is_same_v<String, decltype(params)>); // No pass by copy
		        b = appendToBuffer(b, params);
		    } (), ...);
			return std::string_view(buffer.data(), buffer.size() - b.size());
		}

		//////////

		String& operator += (const String &p);
		String& operator += (const char* p);
		String& operator += (const wchar_t* p);
		String& operator += (const double &p);
		String& operator += (const int &p);
		String& operator += (const Character &p);

		bool operator== (const String& rhp) const = default;
		bool operator== (const char* rhp) const;
		bool operator== (const char8_t* rhp) const;
		bool operator== (std::string_view rhp) const;
		bool operator== (std::u8string_view rhp) const;
		bool operator!= (const String& rhp) const = default;
		bool operator!= (const char* rhp) const;
		bool operator!= (const char8_t* rhp) const;
		bool operator!= (std::string_view rhp) const;
		bool operator!= (std::u8string_view rhp) const;
		std::strong_ordering operator<=> (const String& rhp) const;
		std::strong_ordering operator<=> (const char* rhp) const;
		std::strong_ordering operator<=> (const char8_t* rhp) const;
		std::strong_ordering operator<=> (std::string_view rhp) const;
		std::strong_ordering operator<=> (std::u8string_view rhp) const;

		operator std::string_view() const noexcept { return str; }
		explicit operator std::u8string_view() const noexcept { return std::u8string_view(reinterpret_cast<const char8_t*>(str.c_str()), str.length()); }
		const String& toString() const { return *this; }

		[[nodiscard]] size_t getSizeBytes() const;

		[[nodiscard]] static const String& emptyString();

		void secureClear();

		static void secureClearData(void* data, size_t size);

	private:
		Character* getCharPointer(size_t pos);
		static size_t UTF8toUTF16(std::string_view, wchar_t *utf16);
		static size_t UTF16toUTF8(std::wstring_view, char *utf8);
		static size_t UTF32toUTF8(std::u32string_view utf32, char *utf8);

		std::string str;
	};

	String operator+ (const String& lhp, const String& rhp);
	String operator+ (const std::string_view& lhp, const String& rhp);
	String operator+ (const String& lhp, const std::string_view& rhp);
	String operator+ (const char* lhp, const String& rhp);
	String operator+ (const String& lhp, const char* rhp);
	std::ostream& operator<< (std::ostream& os, const String& rhp);
	std::istream& operator>> (std::istream& is, String& rhp);

	std::strong_ordering operator<=> (std::string_view lhp, const String& rhp);
	std::strong_ordering operator<=> (std::u8string_view lhp, const String& rhp);
	std::strong_ordering operator<=> (const std::basic_string_view<char32_t>& lhp, const StringUTF32& rhp);
	bool operator== (std::string_view lhp, const String& rhp);
	bool operator== (std::u8string_view lhp, const String& rhp);
	bool operator== (const std::basic_string_view<char32_t>& lhp, const StringUTF32& rhp);
	bool operator!= (std::string_view lhp, const String& rhp);
	bool operator!= (std::u8string_view lhp, const String& rhp);
	bool operator!= (const std::basic_string_view<char32_t>& lhp, const StringUTF32& rhp);

	using StringArray = Vector<String>;

	template <typename T>
	constexpr void UnicodeViewIterator<T>::next()
	{
		nextValue = String::extractNextCharacterAndAdvance(view).value_or(0);
	}
}

namespace std {
	template<>
	struct hash<Halley::String>
	{
		using hash_type = std::hash<std::string_view>;
		using is_transparent = void;

		size_t operator()(const Halley::String& s) const noexcept { return hash_type()(s.cppStr());	}
		size_t operator()(const std::string& s) const noexcept { return hash_type()(s); }
		size_t operator()(const std::string_view s) const noexcept { return hash_type()(s); }
		size_t operator()(const char* s) const noexcept { return hash_type()(s); }
	};
}
