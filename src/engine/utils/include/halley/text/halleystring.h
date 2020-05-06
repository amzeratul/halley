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
#include <sstream>
#include <halley/data_structures/vector.h>
#include <gsl/gsl_assert>
#include <iomanip>
#include <cstdint>

namespace Halley {

	typedef char Character;
	typedef wchar_t utf16type;
	typedef char32_t utf32type;
	typedef std::wstring StringUTF16;
	typedef std::u32string StringUTF32;

	// String class
	class String {
	public:
		const static size_t npos = size_t(-1);

		String();
		String(const char* utf8);
		String(const char* utf8,size_t bytes);
		String(const std::basic_string<Character>& str);
		String(const String& str) noexcept;
		String(String&& str) noexcept;

		explicit String(const wchar_t* utf16);
		explicit String(const StringUTF32 &utf32);
		explicit String(char character);
		explicit String(wchar_t character);
		explicit String(int character);
		explicit String(float number);
		explicit String(double number);

		String& operator=(const char* utf8);
		String& operator=(std::basic_string<Character>&& str);
		String& operator=(const std::basic_string<Character>& str);
		String& operator=(String&& str) noexcept;
		String& operator=(const String& str);

		operator std::string() const;
		
		bool isEmpty() const;
		size_t length() const;
		
		void setSize(size_t size);
		void truncate(size_t size);

		String& trim(bool fromRight);
		String& trimBoth();

		bool contains(const String& string) const;
		size_t find(String str) const;

		String replaceAll(const String& before, const String& after) const;
		String replaceOne(const String& before, const String& after) const;
		void shrink();

		String left(size_t n) const;
		String right(size_t n) const;
		String mid(size_t start,size_t count=npos) const;

		bool startsWith(const String& string,bool caseSensitive=true) const;
		bool endsWith(const String& string,bool caseSensitive=true) const;

		void writeText(const Character* src,size_t len,size_t &pos);
		void writeChar(const Character &src,size_t &pos);
		void writeNumber(Character *temp,int number,int pad,size_t &pos);

		bool isNumber() const;
		bool isInteger() const;

		String asciiLower() const;
		String asciiUpper() const;
		void asciiMakeUpper();
		void asciiMakeLower();
		bool asciiCompareNoCase(const Character *src) const;

		void appendCharacter(int unicode);

		// Convert a string to a number
		int toInteger() const { return std::stoi(str); }
		long long toInteger64() const { return std::stoll(str); }
		unsigned long toUInteger() const { return std::stoul(str); }
		unsigned long long toUInteger64() const { return std::stoull(str); }
		float toFloat() const;
		double toDouble() const;
		int subToInteger(size_t start,size_t end) const;

		// std::string methods
		const char* c_str() const;
		String substr(size_t pos, size_t len=npos) const;
		size_t find(Character character, size_t pos=0) const;
		size_t find(const char* str, size_t pos=0) const;
		size_t find_last_of(char character) const;
		size_t size() const;
		const char& operator[](size_t pos) const;
		char& operator[](size_t pos);

		static const Character* stringPtrTrim(Character *chr,size_t len,size_t startPos);
		static const Character* stringTrim(String &str,size_t startPos);

		// Number tidy up functions
		static String prettyFloat(String src);
		static String prettySize(long long bytes);

		// Unicode routines
		StringUTF16 getUTF16() const;
		StringUTF32 getUTF32() const;
		size_t getUTF32Len() const;

		// Static unicode routines
		static size_t getUTF8Len(const wchar_t *utf16);
		static size_t getUTF8Len(const StringUTF32 &utf32);
		static size_t getUTF16Len(const StringUTF32 &utf32);

		inline std::string& cppStr() { return str; }
		inline const std::string& cppStr() const { return str; }

		Vector<String> split(char delimiter) const;
		Vector<String> split(String delimiter) const;
		static String concatList(const Vector<String>& list, String separator);

		//////////

		String& operator += (const String &p);
		String& operator += (const char* p);
		String& operator += (const wchar_t* p);
		String& operator += (const double &p);
		String& operator += (const int &p);
		String& operator += (const Character &p);

		bool operator== (const String& rhp) const;
		bool operator!= (const String& rhp) const;
		bool operator< (const String& rhp) const;
		bool operator> (const String& rhp) const;
		bool operator<= (const String& rhp) const;
		bool operator>= (const String& rhp) const;

	private:
		Character* getCharPointer(size_t pos);
		static size_t UTF8toUTF16(const char *utf8,wchar_t *utf16);
		static size_t UTF16toUTF8(const wchar_t *utf16,char *utf8);
		static size_t UTF32toUTF8(const utf32type *utf32,char *utf8);

		std::string str;
	};

	String operator+ (const String& lhp, const String& rhp);
	std::ostream& operator<< (std::ostream& os, const String& rhp);
	std::istream& operator>> (std::istream& is, String& rhp);

	using StringArray = Vector<String>;

	
}

namespace std {
	template<>
	struct hash<Halley::String>
	{
		size_t operator()(const Halley::String& s) const noexcept
		{
			return std::hash<std::string>()(s.cppStr());
		}
	};
}
