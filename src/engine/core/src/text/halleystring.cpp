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

#ifdef _MSC_VER
#pragma warning(disable: 4748)
#endif

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <winbase.h>
#endif

#include "halley/text/halleystring.h"
#include "halley/support/exception.h"
#include "halley/support/assert.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include "halley/support/assert.h"
#include "halley/text/string_converter.h"
#include "../contrib/fast_float/fast_float.h"

using namespace Halley;

namespace {
	std::string_view toStringView(std::u8string_view str)
	{
		return std::string_view(reinterpret_cast<const char*>(str.data()), str.length());
	}

	bool isSpace(char chr)
	{
		return (chr == ' ' || chr == '\t' || chr == '\n' || chr == '\r');
	}

	constexpr std::string_view trimSpaces(std::string_view str, bool trimLeft = true, bool trimRight = true)
	{
		const size_t n = str.length();
		size_t leftTrim = 0;
		size_t rightTrim = 0;

		if (trimLeft) {
			for (size_t i = 0; i < n; ++i) {
				if (isSpace(str[i])) {
					leftTrim = i + 1;
				} else {
					break;
				}
			}
		}
		if (trimRight && leftTrim != n) {
			for (size_t i = 0; i < n; ++i) {
				if (isSpace(str[n - i - 1])) {
					rightTrim = i + 1;
				} else {
					break;
				}
			}
		}
		
		return str.substr(leftTrim, n - leftTrim - rightTrim);
	}
}

String::String()
{
}

String::String(const char* utf8)
	: str(utf8 ? utf8 : "")
{
}

String::String(const char* utf8, size_t bytes)
{
	str.resize(bytes);
	if (bytes > 0) {
		memcpy(getCharPointer(0), utf8, bytes);
	}
}

String::String(const char8_t* utf8)
	: str(toStringView(utf8))
{
}

String::String(std::string_view strView)
	: str(strView)
{
}

String::String(std::u8string_view strView)
	: str(toStringView(strView))
{
}

String::String(const std::basic_string<Character>& str)
	: str(str)
{
}

String::String(std::basic_string<Character>&& str)
	: str(std::move(str))
{
}

String::String(const wchar_t* utf16)
{
	size_t len = getUTF8Len(utf16);
	str.resize(len);
	if (len > 0) {
		UTF16toUTF8(utf16, getCharPointer(0));
	}
}

String::String(std::wstring_view utf16)
{
	size_t len = getUTF8Len(utf16);
	str.resize(len);
	if (len > 0) {
		UTF16toUTF8(utf16, getCharPointer(0));
	}
}

String::String(std::u32string_view utf32)
{
	size_t len = getUTF8Len(utf32);
	str.resize(len);
	if (len > 0) {
		UTF32toUTF8(utf32, getCharPointer(0));
	}
}

String::String(const StringUTF32& utf32)
{
	size_t len = getUTF8Len(utf32);
	str.resize(len);
	if (len > 0) {
		UTF32toUTF8(utf32, getCharPointer(0));
	}
}

String::String(const String& other) noexcept
{
	str = other.str;
}

String::String(String&& other) noexcept
{
	if (this != &other) {
		str = std::move(other.str);
	}
}

String::String(char character)
{
	*this = std::string(1,character);
}

String::String(wchar_t character)
{
	wchar_t tmp[2];
	tmp[0] = character;
	tmp[1] = 0;
	*this = String(tmp);
}

String::String(char32_t utf32Character)
{
	char32_t tmp[2];
	tmp[0] = utf32Character;
	tmp[1] = 0;
	*this = String(StringUTF32(tmp, 2));
}

String::String(int character)
{
	StringUTF32 tmp;
	tmp.append(1,character);
	*this = String(tmp);
}

String::String(float number)
{
	*this = Halley::toString(number);
}


String::String(double number)
{
	*this = Halley::toString(number);
}

String::String(const Bytes& bytes)
{
	setSize(bytes.size());
	memcpy(&(*this)[0], bytes.data(), bytes.size());
}

String& String::operator=(const char* utf8) {
	if (utf8) {
		str = utf8;
	} else {
		str = "";
	}
	return *this;
}

String& String::operator=(std::basic_string<Character>&& s)
{
	str = std::move(s);
	return *this;
}

String& String::operator=(const std::basic_string<Character>& s) {
	str = s;
	return *this;
}

String& String::operator=(String&& s) noexcept
{
	str = std::move(s.str);
	return *this;
}

String& String::operator=(const String& s) {
	str = s.str;
	return *this;
}

Character* String::getCharPointer(size_t pos)
{
	return &operator[](pos);
}

void String::setSize(size_t size)
{
	str.resize(size);
}

void String::truncate(size_t size)
{
	operator[](size) = 0;
	setSize(size);
}

String& String::trim(bool fromRight)
{
	const auto view = std::string_view(*this);
	const auto res = trimSpaces(view, !fromRight, fromRight);
	if (view != res) {
		*this = res;
	}

	return *this;
}

String& String::trimBoth()
{
	const auto view = std::string_view(*this);
	const auto res = trimSpaces(view);
	if (view != res) {
		*this = res;
	}

	return *this;
}

bool String::contains(Character chr) const
{
	return str.find(chr) != npos;
}

bool String::contains(std::string_view string, bool caseSensitive, bool paramIsPreLowercased) const
{
	return find(string, caseSensitive, paramIsPreLowercased) != npos;
}

bool String::contains(std::u8string_view string, bool caseSensitive, bool paramIsPreLowercased) const
{
	return find(string, caseSensitive, paramIsPreLowercased) != npos;
}

String String::left(size_t n) const
{
	return String(str.substr(0, n));
}

String String::right(size_t n) const
{
	size_t len = size();
	return String(str.substr(len - n, n));
}

String String::mid(size_t start, size_t count) const
{
	return String(str.substr(start, count));
}

bool String::startsWith(std::string_view string, bool caseSensitive) const
{
	if (caseSensitive) {
		return std::string_view(str).starts_with(string);
	} else {
		return asciiLower().startsWith(String(string).asciiLower(), true);
	}
}

bool String::startsWith(std::u8string_view string, bool caseSensitive) const
{
	return startsWith(toStringView(string), caseSensitive);
}

bool String::startsWithAnyOf(gsl::span<const String> strings, bool caseSensitive) const
{
	for (const auto& str: strings) {
		if (startsWith(str, caseSensitive)) {
			return true;
		}
	}
	return false;
}


bool String::endsWith(std::string_view string, bool caseSensitive) const
{
	if (caseSensitive) {
		return std::string_view(str).ends_with(string);
	} else {
		return asciiLower().endsWith(String(string).asciiLower(), true);
	}
}

bool String::endsWith(std::u8string_view string, bool caseSensitive) const
{
	return endsWith(toStringView(string), caseSensitive);
}


void String::writeText(const Character* src,size_t len,size_t &pos)
{
	char *dst = getCharPointer(pos);
	memcpy(dst,src,len*sizeof(Character));
	pos += len;
}


void String::writeChar(const Character &src,size_t &pos)
{
	char *dst = getCharPointer(pos);
	*dst = src;
	pos++;
}


void String::writeNumber(Character *temp,int number,int pad,size_t &pos)
{
	char *dst = getCharPointer(pos);

	// Write number backwards first
	int div, value;
	size_t len;
	for (len=0;true;len++) {
		div = number / 10;
		value = number - (div*10);
		temp[len] = Character(value + '0');
		if (!div) break;
		number = div;
	}
	len++;

	// Pad with zeroes
	pad -= int(len);
	for (int i=0;i<pad;i++) {
		*dst++ = '0';
		pos++;
	}

	// Write number
	for (size_t i=0;i<len;i++) {
		*dst++ = temp[len-i-1];
		pos++;
	}
}


bool String::asciiCompareNoCase(const Character *src) const
{
	unsigned char mask = 0xDF; // 0xDF
	unsigned char c1,c2;
	size_t len = size();
	for (size_t i=0;i<len;i++) {
		// Abort on end of string 2
		c2 = static_cast<unsigned char>(operator[](i));
		if (!c2) return false;

		// Upper case both, this ONLY WORKS FOR ASCII
		c1 = static_cast<unsigned char>(src[i]) & mask;
		c2 = c2 & mask;

		// Check them
		if (c1 != c2) return false;
	}

	// Equal strings
	return true;
}

bool String::isNumber(std::string_view str)
{
	auto trimmed = trimSpaces(str);

	bool foundSeparator = false;
	bool foundDigit = false;
	bool lastFound = false;
	size_t i = 0;

	for (const auto& cur: trimmed) {
		if (lastFound) {
			return false;
		}

		if (cur >= '0' && cur <= '9') {
			foundDigit = true;
		} else if (cur == '.' || cur == ',') {
			if (foundSeparator) {
				return false;
			}
			foundSeparator = true;
		} else if (cur == '-' || cur == '+') {
			if (i != 0) {
				return false;
			}
		} else if (cur == 'f') {
			lastFound = true;
		} else {
			return false;
		}

		i++;
	}
	return foundDigit;
}

bool String::isNumber() const
{
	return isNumber(std::string_view(*this));
}

bool String::isInteger(std::string_view str)
{
	auto trimmed = trimSpaces(str);
	bool hasDigit = false;
	int i = 0;
	for (const auto& cur: trimmed) {
		if (cur == '-' || cur == '+') {
			if (i != 0) {
				return false;
			}
		} else if (cur < '0' || cur > '9') {
			return false;
		} else {
			hasDigit = true;
		}
		i++;
	}
	return hasDigit;
}

bool String::isInteger() const
{
	return isInteger(std::string_view(*this));
}

bool String::isAlphanumeric(uint32_t character)
{
	return (character >= 'A' && character <= 'Z')
		|| (character >= 'a' && character <= 'z')
		|| (character >= '0' && character <= '9');
}


//

String String::asciiLower() const
{
	String tmp(*this);
	tmp.asciiMakeLower();
	return tmp;
}

String String::asciiUpper() const {
	String tmp(*this);
	tmp.asciiMakeUpper();
	return tmp;
}

void String::asciiMakeUpper()
{
	if (length() > 0) {
		char* s = getCharPointer(0);
		for (int i=0; s[i]; s++) {
			char cur = s[i];
			if (cur >= 'a' && cur <= 'z') s[i] -= 32;
		}
	}
}

void String::asciiMakeLower() 
{
	if (length() > 0) {
		char* s = getCharPointer(0);
		for (int i=0; s[i]; s++) {
			char cur = s[i];
			if (cur >= 'A' && cur <= 'Z') s[i] += 32;
		}
	}
}


///////////////

std::string_view String::concatStringViewsInBuffer(gsl::span<char> buffer, gsl::span<const std::string_view> views, std::string_view separator)
{
	size_t writePos = 0;
	const size_t n = views.size();

	const auto& concat = [&](std::string_view s)
	{
		if (buffer.size() < writePos + s.length()) {
			throw Exception("Buffer not large enough for string concatenation", HalleyExceptions::Utils);
		}
		memcpy(&buffer[writePos], s.data(), s.length());
		writePos += s.length();
	};

	for (size_t i = 0; i < n; ++i) {
		concat(views[i]);
		if (i + 1 != n) {
			concat(separator);
		}
	}

	return std::string_view(buffer.data(), writePos);
}

String& String::operator += (const String &p)
{
	str.append(std::string_view(p));
	return *this;
}

#ifdef WX_COMPAT
String String::operator += (const wxString &p)
{
	return operator +=(String(p));
}
#endif

String& String::operator += (const char* p)
{
	str.append(p);
	return *this;
}

String& String::operator += (const wchar_t* p)
{
	return operator +=(String(p));
}

String& String::operator += (const double &p)
{
	return operator +=(Halley::toString(p));
}

String& String::operator += (const int &p)
{
	return operator +=(Halley::toString(p));
}

String& String::operator += (const Character &p)
{
	str.append(1,p);
	return *this;
}

bool String::operator==(const char* rhp) const
{
	return std::string_view(str) == rhp;
}

bool String::operator==(const char8_t* rhp) const
{
	return std::string_view(str) == rhp;
}

bool String::operator==(std::string_view rhp) const
{
	return std::string_view(str) == rhp;
}

bool String::operator==(std::u8string_view rhp) const
{
	return std::string_view(str) == toStringView(rhp);
}

bool String::operator!=(const char* rhp) const
{
	return std::string_view(str) != rhp;
}

bool String::operator!=(const char8_t* rhp) const
{
	return std::string_view(str) != rhp;
}

bool String::operator!=(std::string_view rhp) const
{
	return std::string_view(str) != rhp;
}

bool String::operator!=(std::u8string_view rhp) const
{
	return std::string_view(str) != toStringView(rhp);
}

void operator <<(double &p1, String &p2)
{
	p1 = std::stof(p2.c_str());
}


////////////////
// Pretty float
String String::prettyFloat(String src, char decimalSeparator)
{
	const auto srcView = std::string_view(src);
	const auto res = prettyFloat(srcView, decimalSeparator);
	if (res != srcView) {
		return String(res);
	} else {
		return src;
	}
}

std::string_view String::prettyFloat(std::string_view src, char decimalSeparator)
{
	if (src.empty()) [[unlikely]] {
		return src;
	}

	bool foundSeparator = false;
	size_t lastGoodIdx = 0;
	const size_t len = src.length();

	for (size_t i = 0; i < len; ++i) {
		if (!foundSeparator) {
			if (src[i] == decimalSeparator) {
				foundSeparator = true;
			} else {
				lastGoodIdx = i;
			}
		} else {
			if (src[i] != '0') {
				lastGoodIdx = i;
			}
		}
	}

	return src.substr(0, lastGoodIdx + 1);
}


///////////////////////////////////////////////
// Get the UTF-8 length out of a UTF-16 string
size_t String::getUTF8Len(std::u16string_view utf16)
{
	size_t len = 0;
	wchar_t curChar = utf16[0];
	for (size_t i = 0; curChar; curChar = utf16[++i]) {
		if (curChar == 0) {
			break;
		}

		if ((curChar & 0xFF80) == 0) {
			len++;
		} else if ((curChar & 0xFC00) == 0xD800) {
			len += 4;
			i++;
		} else if (curChar & 0xF800) {
			len += 3;
		} else if (curChar & 0xFF80) {
			len += 2;
		}
	}

	return len;
}

size_t String::getUTF8Len(std::wstring_view utf16)
{
	size_t len = 0;
	wchar_t curChar = utf16[0];
	for (size_t i = 0; curChar && i < utf16.length(); ++i) {
		curChar = utf16[i];
		if ((curChar & 0xFF80) == 0) {
			len++;
		} else if ((curChar & 0xFC00) == 0xD800) {
			len += 4;
			i++;
		} else if (curChar & 0xF800) {
			len += 3;
		} else if (curChar & 0xFF80) {
			len += 2;
		}
	}

	return len;
}


///////////////////////////////////////////////
// Get the UTF-8 length out of a UTF-32 string
size_t String::getUTF8Len(std::u32string_view str)
{
	const size_t srcLen = str.length();
	size_t len = 0;
	for (size_t i = 0; i < srcLen; ++i) {
		const auto curChar = str[i];
		if (curChar == 0) {
			break;
		}

		if (curChar <= 0x7F) {
			len += 1;
		} else if (curChar <= 0x7FF) {
			len += 2;
		} else if (curChar <= 0xFFFF) {
			len += 3;
		} else if (curChar <= 0x10FFFF) {
			len += 4;
		}
	}

	return len;
}


///////////////////////////
// Convert UTF-16 to UTF-8
size_t String::UTF16toUTF8(std::wstring_view utf16, char *utf8)
{
	size_t value;
	size_t written = 0;

	const size_t len = utf16.length();
	for (size_t i = 0; i < len; ++i) {
		wchar_t curChar = utf16[i];
		if (curChar == 0) {
			break;
		}

		// 1 byte
		if ((curChar & 0xFF80) == 0) {
			utf8[written] = char(curChar);
			if (curChar == 0) break;
			written++;
		}

		// 2 bytes
		else if ((curChar & 0xF800) == 0) {
			utf8[written] = char(((curChar & 0x07C0) >> 6)  | 0xC0);
			utf8[written+1] = char((curChar & 0x003F)       | 0x80);
			written += 2;
		}

		// Surrogate pair UTF-16
		else if ((curChar & 0xFC00) == 0xD800) {
			// Read
			int c0 = curChar;
			int c1 = utf16[i+1];
			value = (((c0 - 0xD800) << 10) | (c1 - 0xDC00)) + 0x10000;
			i++;

			// Write
			utf8[written] = char(((value & 0x1C0000) >> 18)	  | 0xF0);
			utf8[written+1] = char(((value & 0x03F000) >> 12) | 0x80);
			utf8[written+2] = char(((value & 0x000FC0) >> 6)  | 0x80);
			utf8[written+3] = char((value & 0x00003F)         | 0x80);
			written += 4;
		}

		// 3 bytes
		else if (curChar & 0xF800) {
			utf8[written] = char(((curChar & 0xF000) >> 12)   | 0xE0);
			utf8[written+1] = char(((curChar & 0x0FC0) >> 6)  | 0x80);
			utf8[written+2] = char((curChar & 0x003F)         | 0x80);
			written += 3;
		}
	}
	return written;
}

///////////////////////////
// Convert UTF-32 to UTF-8
size_t String::UTF32toUTF8(std::u32string_view utf32, char *utf8)
{
	size_t written = 0;
	const size_t len = utf32.length();

	for (size_t i = 0; i < len; ++i) {
		const auto curChar = utf32[i];
		if (curChar == 0) {
			break;
		}

		// 1 byte
		if (curChar <= 0x7F) {
			utf8[written] = char(curChar);
			if (curChar == 0) break;
			written++;
		}

		// 2 bytes
		else if (curChar <= 0x7FF) {
			utf8[written] = char(((curChar & 0x07C0) >> 6)  | 0xC0);
			utf8[written+1] = char((curChar & 0x003F)       | 0x80);
			written += 2;
		}

		// 3 bytes
		else if (curChar <= 0xFFFF) {
			utf8[written] = char(((curChar & 0xF000) >> 12)   | 0xE0);
			utf8[written+1] = char(((curChar & 0x0FC0) >> 6)  | 0x80);
			utf8[written+2] = char((curChar & 0x003F)         | 0x80);
			written += 3;
		}		

		// 4 bytes
		else if (curChar <= 0x10FFFF) {
			utf8[written] = char(((curChar & 0x1C0000) >> 18)	| 0xF0);
			utf8[written+1] = char(((curChar & 0x03F000) >> 12) | 0x80);
			utf8[written+2] = char(((curChar & 0x000FC0) >> 6)  | 0x80);
			utf8[written+3] = char((curChar & 0x00003F)         | 0x80);
			written += 4;
		}
	}
	return written;
}

size_t String::UTF8toUTF16(std::string_view utf8, wchar_t *utf16)
{
	StringUTF32 str = String(utf8).getUTF32();
	size_t written = 0;
	for (size_t i=0; i<str.size(); i++) {
		int code = str[i];
		if (code <= 0xD7FF || (code >= 0xE000 && code <= 0xFFFF)) {
			utf16[written++] = wchar_t(code);
		} else {
			code -= 0x10000;
			wchar_t high = wchar_t((code >> 10) + 0xD800);
			wchar_t low = wchar_t((code & 0x3FF) + 0xDC00);
			utf16[written++] = high;
			utf16[written++] = low;
		}
	}
	return written;
}

size_t String::getUTF16Len(const StringUTF32& str)
{
	size_t written = 0;
	for (size_t i=0; i<str.size(); i++) {
		int code = str[i];
		if (code <= 0xD7FF || (code >= 0xE000 && code <= 0xFFFF)) {
			written++;
		} else {
			written += 2;
		}
	}
	return written;
}

StringUTF16 String::getUTF16() const
{
	StringUTF16 result;
	StringUTF32 utf32 = getUTF32();
	size_t sz = getUTF16Len(utf32);
	result.resize(sz);
	if (sz > 0) UTF8toUTF16(str.data(), &result[0]);
	return result;
}

StringUTF32 String::getUTF32() const
{
	StringUTF32 result(getUTF32Len(), static_cast<char32_t>(0));

	size_t len = length();
	size_t dst = 0;
	const auto src = std::string_view(*this);
	for (size_t i = 0; i < len;) {
		const auto [c, stride] = extractNextCharacter(src.substr(i));
		result[dst++] = c;
		i += stride;
	}

	return result;
}

size_t String::getUTF32Len() const
{
	return getUTF32Len(*this);
}

size_t String::getUTF8Len(std::string_view utf8)
{
	return utf8.length();
}

size_t String::getUTF32Len(std::string_view str)
{
	size_t len = str.length();
	size_t result = 0;
	for (size_t i = 0; i < len;) {
		const auto [c, stride] = extractNextCharacter(str.substr(i));
		i += stride;
		result++;
	}
	return result;
}

size_t String::getUTF32Len(std::u32string_view str)
{
	return str.length();
}

std::pair<char32_t, int> String::extractNextCharacter(std::string_view str)
{
	uint32_t c0 = static_cast<uint8_t>(str[0]);

	// 1 byte
	if ((c0 >> 7) == 0) {
		return { static_cast<char32_t>(c0), 1 };
	}

	// 2 bytes
	else if ((c0 >> 5) == 0x06) {
		uint32_t c1 = static_cast<uint8_t>(str[1]);
		if ((c1 >> 6) == 0x02) {
			return { ((c0 & 0x1F) << 6) | (c1 & 0x3F), 2 };
		}
	}

	// 3 bytes
	else if ((c0 >> 4) == 0x0E) {
		uint32_t c1 = static_cast<uint8_t>(str[1]);
		uint32_t c2 = static_cast<uint8_t>(str[2]);
		if ((c1 >> 6) == 0x02 && (c2 >> 6) == 0x02) {
			return { ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F), 3 };
		}
	}

	// 4 bytes
	else if ((c0 >> 3) == 0x1E) {
		uint32_t c1 = static_cast<uint8_t>(str[1]);
		uint32_t c2 = static_cast<uint8_t>(str[2]);
		uint32_t c3 = static_cast<uint8_t>(str[3]);
		if ((c1 >> 6) == 0x02 && (c2 >> 6) == 0x02 && (c3 >> 6) == 0x02) {
			return { ((c0 & 0x07) << 18) | ((c1 & 0x03F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F), 4 };
		}
	}

	return { 0, 1 };
}

std::pair<char32_t, int> String::extractNextCharacter(std::u32string_view str)
{
	return { str[0], 1 };
}

std::optional<char32_t> String::extractNextCharacterAndAdvance(std::string_view& str)
{
	if (str.empty()) {
		return std::nullopt;
	}

	auto [c, advance] = extractNextCharacter(str);
	if (c == 0 && advance == 0) {
		return std::nullopt;
	} else {
		str = str.substr(advance);
		return c;
	}
}

UnicodeView<char> String::getUnicodeView() const
{
	return UnicodeView<char>(std::string_view(*this));
}

std::optional<char32_t> String::extractNextCharacterAndAdvance(std::u32string_view& str)
{
	auto [c, advance] = extractNextCharacter(str);
	if (c == 0 && advance == 0) {
		return std::nullopt;
	} else {
		str = str.substr(advance);
		return c;
	}
}

String String::prettySize(uint64_t bytes)
{
	uint64_t value = bytes;
	uint64_t steps = 0;
	uint64_t div = 1;
	while (steps < 5 && value > 1024) {
		value >>= 10;
		div <<= 10;
		steps++;
	}
	String suffixes[] = { " B", " kB", " MB", " GB", " TB", " PB" };
	int prec = steps == 0 ? 0 : 2;
	return Halley::toString(double(bytes) / double(div), prec) + suffixes[steps];
}

String String::integerAddThousandsSeparator(std::string_view str, char thousandsSeparator)
{
	std::stringstream ss2;

	const size_t signLen = str[0] == '-' ? 1 : 0;
	const size_t totalLen = str.length();
	const size_t numLen = totalLen - signLen;
	size_t firstBlockLen = numLen % 3;
	if (firstBlockLen == 0) {
		firstBlockLen = 3;
	}
	firstBlockLen += signLen;

	size_t pos = 0;
	size_t remaining = totalLen;
	for (size_t len = firstBlockLen; remaining > 0; ) {
		ss2 << std::string_view(str).substr(pos, len);
		pos += len;
		remaining -= len;
		len = 3;

		if (remaining > 0) {
			ss2 << thousandsSeparator;
		}
	}
	return ss2.str();
}

Vector<String> String::split(char delimiter, size_t limit) const
{
	Vector<String> result;
	
	size_t startPos = 0;
	while (true) {
		size_t endPos = result.size() + 1 != limit ? find(delimiter, startPos) : npos;
		if (endPos == npos) {
			// No more delimiters
			result.push_back(substr(startPos));
			break;
		} else {
			result.push_back(substr(startPos, endPos-startPos));
			startPos = endPos+1;
		}
	}

	HalleyAssertDev(!result.empty());
	return result;
}

Vector<String> String::split(std::string_view delimiter, size_t limit) const
{
	Vector<String> result;
	
	size_t size = delimiter.size();
	size_t startPos = 0;
	const char* cStr = delimiter.data();
	while (true) {
		size_t endPos = result.size() + 1 != limit ? find(cStr, startPos) : npos;
		if (endPos == npos) {
			// No more delimiters
			result.push_back(substr(startPos));
			break;
		} else {
			result.push_back(substr(startPos, endPos-startPos));
			startPos = endPos + size;
		}
	}

	HalleyAssertDev(!result.empty());
	return result;
}

std::pair<std::string_view, std::string_view> String::split(std::string_view src, char delimeter)
{
	if (const auto pos = src.find(delimeter); pos != std::string_view::npos) {
		return { src.substr(0, pos), src.substr(pos + 1) };
	} else {
		return { src, {} };
	}
}

std::string_view String::splitAndAdvance(std::string_view& src, char delimeter)
{
	auto [res, next] = split(src, delimeter);
	src = next;
	return res;
}

gsl::span<std::string_view> String::splitToBuffer(std::string_view src, char delimeter, gsl::span<std::string_view> buffer)
{
	size_t i;
	for (i = 0; i < buffer.size(); ++i) {
		auto [str, remainder] = split(src, delimeter);
		buffer[i] = str;
		if (remainder.empty()) {
			return buffer.subspan(0, i + 1);
		}
		src = remainder;
	}
	throw Exception("Buffer not big enough to split string: \"" + String(src) + "\"", HalleyExceptions::Utils);
}

String String::concatList(gsl::span<const String> list, std::string_view separator)
{
	std::stringstream ss;
	for (size_t i = 0; i < list.size(); i++) {
		if (i != 0) {
			ss << separator;
		}
		ss << list[i].cppStr();
	}
	return ss.str();
}

void String::appendCharacter(int unicode)
{
	// Backspace
	if (unicode == 8 || unicode == 42) {
		StringUTF32 utf32 = getUTF32();
		if (!utf32.empty()) {
			utf32.pop_back();
		}
		*this = String(utf32);
	} else {
		StringUTF32 utf32;
		utf32 += static_cast<char32_t>(unicode);
		*this += String(utf32);
	}
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wnan-infinity-disabled"
#endif

namespace {
	template <typename T>
	T stringToInteger(std::string_view str, int base = 10)
	{
		str = trimSpaces(str);

		if (str.starts_with("+")) [[unlikely]] {
			str = str.substr(1);
		}

		if (str.starts_with("0x") || str.starts_with("0X")) [[unlikely]] {
			base = 16;
			str = str.substr(2);
		}

		T value;
		const fast_float::from_chars_result result = fast_float::from_chars(str.data(), str.data() + str.length(), value, base);
		if (result.ec == std::errc::invalid_argument) [[unlikely]] {
			throw Exception("Unable to convert string \"" + String(str) + "\" to integer: not a number", HalleyExceptions::Utils);
		} else if (result.ec == std::errc::result_out_of_range) [[unlikely]] {
			throw Exception("Unable to convert string \"" + String(str) + "\" to integer: out of range", HalleyExceptions::Utils);
		}
		return value;
	}

	template <typename T>
	T stringToFloat(std::string_view str)
	{
		str = trimSpaces(str);

		if (str == ".inf") [[unlikely]] {
			return std::numeric_limits<T>::infinity();
		} else if (str == "-.inf") [[unlikely]] {
			return -std::numeric_limits<T>::infinity();
		} else if (str == ".nan") [[unlikely]] {
			return std::numeric_limits<T>::quiet_NaN();
		} else if (str.empty()) [[unlikely]] {
			return static_cast<T>(0);
		}

		if (str.front() == '+') [[unlikely]] {
			str = str.substr(1);
		}
		if (str.back() == 'f') [[unlikely]] {
			str = str.substr(0, str.size() - 1);
		}

		T value;
		const fast_float::from_chars_result result = fast_float::from_chars(str.data(), str.data() + str.length(), value);
		if (result.ec == std::errc::invalid_argument) [[unlikely]] {
			throw Exception("Unable to convert string \"" + String(str) + "\" to float: not a number", HalleyExceptions::Utils);
		} else if (result.ec == std::errc::result_out_of_range) [[unlikely]] {
			throw Exception("Unable to convert string \"" + String(str) + "\" to float: out of range", HalleyExceptions::Utils);
		}

		return value;
	}
}

int32_t String::toInteger() const
{
	return stringToInteger<int32_t>(*this);
}

int64_t String::toInteger64() const
{
	return stringToInteger<int64_t>(*this);
}

uint32_t String::toUInteger() const
{
	return stringToInteger<uint32_t>(*this);
}

uint64_t String::toUInteger64() const
{
	return stringToInteger<uint64_t>(*this);
}

float String::toFloat() const
{
	return stringToFloat<float>(*this);
}

double String::toDouble() const
{
	return stringToFloat<double>(*this);
}

int String::subToInteger(size_t start, size_t end) const
{
	return toInteger(std::string_view(*this).substr(start, end - start));
}

int32_t String::toInteger(std::string_view str)
{
	return stringToInteger<int32_t>(str);
}

std::optional<int32_t> String::tryToInteger(std::string_view str)
{
	if (isInteger(str)) {
		return toInteger(str);
	} else {
		return std::nullopt;
	}
}

int64_t String::toInteger64(std::string_view str)
{
	return stringToInteger<int64_t>(str);
}

uint32_t String::toUInteger(std::string_view str)
{
	return stringToInteger<uint32_t>(str);
}

uint64_t String::toUInteger64(std::string_view str)
{
	return stringToInteger<uint64_t>(str);
}

float String::toFloat(std::string_view str)
{
	return stringToFloat<float>(str);
}

std::optional<float> String::tryToFloat(std::string_view str)
{
	if (isNumber(str)) {
		return toFloat(str);
	} else {
		return std::nullopt;
	}
}

double String::toDouble(std::string_view str)
{
	return stringToFloat<double>(str);
}

std::optional<double> String::tryToDouble(std::string_view str)
{
	if (isNumber(str)) {
		return toDouble(str);
	} else {
		return std::nullopt;
	}
}

String String::replaceAll(std::string_view before, std::string_view after) const
{
	const size_t pos = find(before);
	if (pos == std::string::npos) {
		return *this;
	} else {
		const size_t len = before.length();
		return substr(0, pos) + after + substr(pos + len).replaceAll(before, after);
	}
}

String String::replaceAll(std::u8string_view before, std::u8string_view after) const
{
	return replaceAll(toStringView(before), toStringView(after));
}

String String::replaceOne(std::string_view before, std::string_view after) const
{
	const size_t pos = find(before);
	if (pos == std::string::npos) {
		return *this;
	} else {
		const size_t len = before.length();
		return substr(0, pos) + after + substr(pos + len);
	}
}

String String::replaceOne(std::u8string_view before, std::u8string_view after) const
{
	return replaceOne(toStringView(before), toStringView(after));
}

void String::shrink()
{
	str.shrink_to_fit();
}

size_t String::find(std::string_view s, bool caseSensitive, bool paramIsPreLowercased) const
{
	if (s.length() > length()) {
		return std::string::npos;
	}

	if (caseSensitive) {
		// Case-sensitive
		return str.find(s);
	} else if (paramIsPreLowercased) {
		// Case-insensitive, param  is lower case
		return asciiLower().find(s);
	} else {
		// Case-insensitive, param is unknown
		return asciiLower().find(String(s).asciiLower());
	}
}

size_t String::find(std::u8string_view s, bool caseSensitive, bool paramIsPreLowercased) const
{
	return find(toStringView(s), caseSensitive, paramIsPreLowercased);
}

std::ostream& Halley::operator<< (std::ostream& os, const String& rhp)
{
	os << rhp.cppStr();
	return os;
}

std::istream& Halley::operator>> (std::istream& is, String& rhp)
{
	std::string str;
	is >> str;
	rhp = str;
	return is;
}

std::strong_ordering Halley::operator<=>(std::string_view lhp, const String& rhp)
{
	return lhp <=> std::string_view(rhp);
}

std::strong_ordering Halley::operator<=>(std::u8string_view lhp, const String& rhp)
{
	return toStringView(lhp) <=> std::string_view(rhp);
}

std::strong_ordering Halley::operator<=>(const std::basic_string_view<char32_t>& lhp, const StringUTF32& rhp)
{
	return lhp <=> std::basic_string_view<char32_t>(rhp);
}

bool Halley::operator== (std::string_view lhp, const String& rhp) 
{
	return lhp == std::string_view(rhp);
}

bool Halley::operator== (std::u8string_view lhp, const String& rhp) 
{
	return lhp == std::u8string_view(rhp);
}

bool Halley::operator==(const std::basic_string_view<char32_t>& lhp, const StringUTF32& rhp)
{
	return lhp == std::u32string_view(rhp);
}

bool Halley::operator!= (std::string_view lhp, const String& rhp) 
{
	return lhp != std::string_view(rhp);
}

bool Halley::operator!= (std::u8string_view lhp, const String& rhp) 
{
	return lhp != std::u8string_view(rhp);
}

bool Halley::operator!=(const std::basic_string_view<char32_t>& lhp, const StringUTF32& rhp)
{
	return lhp != std::u32string_view(rhp);
}


size_t String::getSizeBytes() const
{
	return str.capacity();
}

const String& String::emptyString()
{
	static String str;
	return str;
}

namespace {
	typedef void* (*memset_t)(void*, int, size_t);

	static volatile memset_t memset_func = memset;

	void cleanse(void* ptr, size_t len) {
		memset_func(ptr, 0, len);
	}
}

void String::secureClear()
{
	str.resize(str.capacity(), 0);
	secureClearData(str.data(), str.size());
	str.clear();
}

void String::secureClearData(void* data, size_t size)
{
	if (size == 0) {
		return;
	}

#ifdef WIN32
	SecureZeroMemory(data, size);
#else
	cleanse(data, size);
#endif
}

String Halley::operator+ (const String& lhp, const String& rhp)
{
	return String(lhp.cppStr() + rhp.cppStr());
}

String Halley::operator+(const std::string_view& lhp, const String& rhp)
{
	return String(std::string(lhp) + rhp.cppStr());
}

String Halley::operator+(const String& lhp, const std::string_view& rhp)
{
	return String(lhp.cppStr() + std::string(rhp));
}

String Halley::operator+(const char* lhp, const String& rhp)
{
	return String(std::string(lhp) + rhp.cppStr());
}

String Halley::operator+(const String& lhp, const char* rhp)
{
	return String(lhp.cppStr() + std::string(rhp));
}

std::strong_ordering String::operator<=>(const String& rhp) const
{
	return str <=> rhp.str;
}

std::strong_ordering String::operator<=>(const char* rhp) const
{
	return str <=> rhp;
}

std::strong_ordering String::operator<=>(const char8_t* rhp) const
{
	return str <=> rhp;
}

std::strong_ordering String::operator<=>(std::string_view rhp) const
{
	return str <=> rhp;
}

std::strong_ordering String::operator<=>(std::u8string_view rhp) const
{
	return *this <=> toStringView(rhp);
}

String::operator std::string() const
{
	return str;
}

const char* String::c_str() const
{
	return str.c_str();
}

String String::substr(size_t pos, size_t len) const
{
	if (pos >= str.size()) {
		return "";
	}
	return str.substr(pos, len);
}

size_t String::find(Character character, size_t pos) const
{
	return str.find(character, pos);
}

size_t String::find(const char* s, size_t pos) const
{
	return str.find(s, pos);
}

size_t String::find_last_of(char character) const
{
	return str.find_last_of(character);
}

size_t String::size() const
{
	return str.size();
}

const char& String::operator[](size_t pos) const
{
	return str[pos];
}

char& String::operator[](size_t pos)
{
	return str[pos];
}

Bytes String::toBytes() const
{
	Bytes result;
	result.resize(length());
	memcpy(result.data(), c_str(), length());
	return result;
}

gsl::span<const char> String::asSpan() const
{
	return gsl::span<const char>(&(*this)[0], length());
}

gsl::span<char> String::asSpan()
{
	return gsl::span<char>(&(*this)[0], length());
}

gsl::span<const std::byte> String::asByteSpan() const
{
	return gsl::as_bytes(asSpan());
}

gsl::span<std::byte> String::asWriteableByteSpan()
{
	return gsl::as_writable_bytes(asSpan());
}

