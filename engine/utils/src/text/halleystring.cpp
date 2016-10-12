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

#include "halley/text/halleystring.h"
#include "halley/support/exception.h"
#include "halley/support/assert.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <gsl/gsl_assert>

using namespace Halley;


String::String()
{
}


String::String(const char* utf8)
: str(utf8 ? utf8 : "")
{
}


String::String(const char* utf8,size_t bytes)
{
	str.resize(bytes);
	if (bytes > 0) memcpy(getCharPointer(0),utf8,bytes);
	//*GetCharPointer(bytes) = 0;
}


String::String(const std::basic_string<Character>& _str)
: str(_str)
{
}


String::String(const wchar_t* utf16)
{
	size_t len = getUTF8Len(utf16);
	str.resize(len);
	if (len > 0) UTF16toUTF8(utf16, getCharPointer(0));
}


String::String(const StringUTF32 &utf32)
{
	size_t len = getUTF8Len(utf32);
	str.resize(len);
	if (len > 0) UTF32toUTF8(&utf32[0],getCharPointer(0));
}

String::String(const String& other)
{
	str = other.str;
}

String::String(String&& other)
{
	str = std::move(other.str);
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


String::String(int character)
{
	StringUTF32 tmp;
	tmp.append(1,character);
	*this = String(tmp);
	//*this = integerToString(integer);
}


String::String(float number)
{
	*this = toString(number);
}


String::String(double number)
{
	*this = toString(number);
}

String& String::operator=(const char* utf8) {
	str = utf8;
	return *this;
}

String& String::operator=(const std::basic_string<Character>& s) {
	str = s;
	return *this;
}

String& String::operator=(String&& s) {
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


bool String::isEmpty() const
{
	return size()==0;
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


bool IsSpace(char chr)
{
	return (chr == ' ' || chr == '\t' || chr == '\n' || chr == '\r');
}


String& String::trim(bool fromRight)
{
	int len = int(length());
	size_t start = 0;
	size_t n = len;

	if (fromRight) {
		for (int i=len;--i>=0;) {
			if (IsSpace(operator[](i))) n = i;
			else break;
		}
	} else {
		for (int i=0;i<len;i++) {
			if (IsSpace(operator[](i))) start = i+1;
			else break;
		}
		n = len - start;
	}

	*this = String(substr(start,n));
	return *this;
}


String& String::trimBoth()
{
	return trim(true).trim(false);
}


size_t String::length() const
{
	return size();
}


bool String::contains(const String& string) const
{
	return str.find(string) != npos;
}


String String::left(size_t n) const
{
	return String(str.substr(0,n));
}


String String::right(size_t n) const
{
	size_t len = size();
	return String(str.substr(len-n,n));
}


String String::mid(size_t start,size_t count) const
{
	return String(str.substr(start,count));
}


bool String::startsWith(const String& string,bool caseSensitive) const
{
	if (caseSensitive) {
		String tmp = String(str.substr(0, string.size()));
		return str.compare(0, string.size(), string) == 0;
	} else {
		return asciiLower().startsWith(string.asciiLower(), true);
	}
}


bool String::endsWith(const String& string,bool caseSensitive) const
{
	if (caseSensitive) {
		size_t strSize = string.size();
		size_t sz = size();
		if (sz < strSize) return false;
		return str.compare(sz - strSize, strSize, string) == 0;
	} else {
		return asciiLower().endsWith(string.asciiLower(), true);
	}
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


bool String::isNumber() const
{
	bool foundSeparator = false;
	bool foundDigit = false;
	size_t i = 0;
	for (const char *chr = c_str();*chr;chr++) {
		char cur = *chr;

		if (cur >= '0' && cur <= '9') {
			foundDigit = true;
		}

		else if (cur == '.' || cur == ',') {
			if (foundSeparator) return false;
			foundSeparator = true;
		}
				
		else if (cur == '-') {
			if (i != 0) return false;
		}

		else {
			return false;
		}

		i++;
	}
	return foundDigit;
}


bool String::isInteger() const
{
	int i=0;
	for (const char *chr = c_str();*chr;chr++) {
		char cur = *chr;
		if (cur == '-') {
			if (i != 0) return false;
		} else if (cur < '0' || cur > '9') {
			return false;
		}
		i++;
	}
	return true;
}


//

const Character* String::stringPtrTrim(Character *chr,size_t len,size_t startPos)
{
	// String metrics
	Character *read = chr;
	size_t start = startPos;
	size_t end = len;
	bool isStart = true;
	bool isEnd = false;
	Character cur;

	// Search for spaces
	for (size_t i=start;i<len;i++) {
		cur = read[i];
		bool isSpace = (cur == ' ');
		if (isStart) {
			if (isSpace) start++;
			else isStart = false;
		}
		if (isEnd) {
			if (!isSpace) isEnd = false;
		}
		else {
			if (isSpace) {
				isEnd = true;
				end = i;
			}
		}
	}

	// Apply changes to pointer
	if (isEnd) chr[end] = 0;
	return chr + start;
}

const Character* String::stringTrim(String &str,size_t startPos)
{
	// Get a pointer to the string data
	Character *chr = const_cast<Character*> (str.c_str());
	return stringPtrTrim(chr,str.length(),startPos);
}

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

String String::operator += (const String &p)
{
	str.append(p);
	return *this;
}

#ifdef WX_COMPAT
String String::operator += (const wxString &p)
{
	str.append(String(p));
	return *this;
}
#endif

String String::operator += (const char* p)
{
	str.append(p);
	return *this;
}

String String::operator += (const wchar_t* p)
{
	str.append(String(p));
	return *this;
}

String String::operator += (const double &p)
{
	str.append(toString(p));
	return *this;
}

String String::operator += (const int &p)
{
	str.append(toString(p));
	return *this;
}

String String::operator += (const Character &p)
{
	str.append(1,p);
	return *this;
}

void operator <<(double &p1,String &p2)
{
	p1 = atof(p2.c_str());
}



//////////////////////////////////
// Convert a string to an integer
int String::toInteger() const
{
	size_t len = length();
	int value = 0;
	int mult = 1;
	int chr;
	bool firstChar = true;
	const char *data = c_str();
	for (size_t i=0;i<len;i++) {
		if (data[i] == ' ') continue;
		chr = int(data[i])-int('0');
		if (chr >= 0 && chr <= 9) value = 10*value+chr;
		else if (firstChar && data[i] == '-') mult = -1;
		else if (firstChar && data[i] == '+') {
			firstChar = false;
			continue;
		}
		else throw Exception("OUT OF RANGE");
		firstChar = false;
	}
	return value * mult;
}

long long String::toInteger64() const
{
	size_t len = length();
	long long value = 0;
	long long mult = 1;
	int chr;
	bool firstChar = true;
	const char *data = c_str();
	for (size_t i=0;i<len;i++) {
		if (data[i] == ' ') continue;
		chr = int(data[i])-int('0');
		if (chr >= 0 && chr <= 9) value = 10*value+chr;
		else if (firstChar && data[i] == '-') mult = -1;
		else if (firstChar && data[i] == '+') {
			firstChar = false;
			continue;
		}
		else throw Exception("OUT OF RANGE");
		firstChar = false;
	}
	return value * mult;
}


float String::toFloat() const
{
	return float(atof(c_str()));
}


//////////////////////////////////////
// Converts a substring to an integer
int String::subToInteger(size_t start,size_t end) const
{
	int value = 0;
	int chr;
	const char *data = c_str();
	for (size_t i=start;i<end;i++) {
		chr = int(data[i])-int('0');
		if (chr >= 0 && chr <= 9) value = 10*value+chr;
	}
	return value;
}


////////////////
// Pretty float
String String::prettyFloat(String src)
{
	if (src.contains(".")) {
		size_t len = src.length();
		while (src.endsWith("0")) {
			len--;
			src.truncate(len);
		}
		if (src.endsWith(".")) {
			len--;
			src.truncate(len);
		}
	}
	return src;
}


///////////////////////////////////////////////
// Get the UTF-8 length out of a UTF-16 string
size_t String::getUTF8Len(const wchar_t *utf16)
{
	size_t len = 0;
	wchar_t curChar = utf16[0];
	for (size_t i=0; curChar; curChar = utf16[++i]) {
		if ((curChar & 0xFF80) == 0) len++;
		else if ((curChar & 0xFC00) == 0xD800) {
			len += 4;
			i++;
		}
		else if (curChar & 0xF800) len += 3;
		else if (curChar & 0xFF80) len += 2;
	}

	return len;
}


///////////////////////////////////////////////
// Get the UTF-8 length out of a UTF-32 string
size_t String::getUTF8Len(const StringUTF32 &str)
{
	const utf32type *utf32 = &str[0];
	size_t len = 0;
	for (int curChar; (curChar = *utf32) != 0; utf32++) {
		if (curChar <= 0x7F) len += 1;
		else if (curChar <= 0x7FF) len += 2;
		else if (curChar <= 0xFFFF) len += 3;
		else if (curChar <= 0x10FFFF) len += 4;
	}

	return len;
}


///////////////////////////
// Convert UTF-16 to UTF-8
size_t String::UTF16toUTF8(const wchar_t *utf16,char *utf8)
{
	wchar_t curChar = utf16[0];
	size_t value;
	size_t written = 0;
	for (size_t i=0;;i++) {
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

		// Get next
		curChar = utf16[i+1];
	}
	return written;
}

///////////////////////////
// Convert UTF-32 to UTF-8
size_t String::UTF32toUTF8(const utf32type *utf32,char *utf8)
{
	size_t written = 0;
	for (int curChar; (curChar = *utf32) != 0; utf32++) {
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

size_t String::UTF8toUTF16(const char *utf8, wchar_t *utf16)
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
	StringUTF32 result(getUTF32Len(), wchar_t(0));

	size_t len = length();
	size_t dst = 0;
	utf32type dstChar = 0;
	for (size_t i=0; i<len;) {
		unsigned int c0 = static_cast<unsigned char>(operator[](i++));

		// 1 byte
		if ((c0 >> 7) == 0) {
			dstChar = utf32type(c0);
		}

		// 2 bytes
		else if ((c0 >> 5) == 0x06) {
			unsigned int c1 = static_cast<unsigned char>(operator[](i++));
			if ((c1 >> 6) == 0x02) {
				dstChar = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
			}
		}

		// 3 bytes
		else if ((c0 >> 4) == 0x0E) {
			unsigned int c1 = static_cast<unsigned char>(operator[](i++));
			unsigned int c2 = static_cast<unsigned char>(operator[](i++));
			if ((c1 >> 6) == 0x02 && (c2 >> 6) == 0x02) {
				dstChar = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
			}
		}

		// 4 bytes
		else if ((c0 >> 3) == 0x1E) {
			unsigned int c1 = static_cast<unsigned char>(operator[](i++));
			unsigned int c2 = static_cast<unsigned char>(operator[](i++));
			unsigned int c3 = static_cast<unsigned char>(operator[](i++));
			if ((c1 >> 6) == 0x02 && (c2 >> 6) == 0x02 && (c3 >> 6) == 0x02) {
				dstChar = ((c0 & 0x07) << 18) | ((c1 & 0x03F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
			}
		}

		result[dst++] = dstChar;
		dstChar = 0;
	}

	return result;
}

size_t Halley::String::getUTF32Len() const
{
	size_t len = length();
	size_t result = 0;
	for (size_t i=0; i<len;) {
		unsigned int c0 = static_cast<unsigned char>(operator[](i++));

		// 1 byte
		if ((c0 >> 7) == 0) {
		}

		// 2 bytes
		else if ((c0 >> 5) == 0x06) {
			i++;
		}

		// 3 bytes
		else if ((c0 >> 4) == 0x0E) {
			i+=2;
		}

		// 4 bytes
		else if ((c0 >> 3) == 0x1E) {
			i+=3;
		}

		result++;
	}
	return result;
}

Halley::String Halley::String::prettySize(long long bytes)
{
	long long value = bytes;
	long long steps = 0;
	long long div = 1;
	while (steps < 5 && value > 1024) {
		value >>= 10;
		div <<= 10;
		steps++;
	}
	String suffixes[] = { "", "kB", "MB", "GB", "TB", "PB" };
	return toString(double(bytes) / double(div)) + " " + suffixes[steps];
}

Vector<String> Halley::String::split(char delimiter) const
{
	Vector<String> result;
	
	size_t startPos = 0;
	while (true) {
		size_t endPos = find(delimiter, startPos);
		if (endPos == npos) {
			// No more delimiters
			result.push_back(substr(startPos));
			break;
		} else {
			result.push_back(substr(startPos, endPos-startPos));
			startPos = endPos+1;
		}
	}

	Ensures(result.size() > 0);
	return result;
}

Vector<String> String::split(String delimiter) const
{
	Vector<String> result;
	
	size_t size = delimiter.size();
	size_t startPos = 0;
	const char* cStr = delimiter.c_str();
	while (true) {
		size_t endPos = find(cStr, startPos);
		if (endPos == npos) {
			// No more delimiters
			result.push_back(substr(startPos));
			break;
		} else {
			result.push_back(substr(startPos, endPos-startPos));
			startPos = endPos + size;
		}
	}

	Ensures(result.size() > 0);
	return result;
}

String String::concatList(const Vector<String>& list, String separator)
{
	std::stringstream ss;
	for (size_t i = 0; i < list.size(); i++) {
		if (i != 0) {
			ss << separator.cppStr();
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
		utf32 = utf32.substr(0, utf32.length()-1);
		*this = String(utf32);
	}

	else {
		StringUTF32 utf32;
		utf32 += wchar_t(unicode);
		*this += String(utf32);
	}
}

void String::replace(String before, String after, bool all)
{
	size_t pos = find(before);
	if (pos != std::string::npos) {
		size_t len = before.length();
		String post = substr(pos+len);
		if (all) post.replace(before, after, true);

		*this = substr(0,pos) + after + post;
	}
}

void String::shrink()
{
	str.reserve(length());
}

size_t String::find(String s) const
{
	return str.find(s.c_str());
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

String Halley::operator+ (const String& lhp, const String& rhp)
{
	return String(lhp.cppStr() + rhp.cppStr());
}

bool String::operator== (const String& rhp) const
{
	return strcmp(str.c_str(), rhp.str.c_str()) == 0;
}

bool String::operator!= (const String& rhp) const
{
	return strcmp(str.c_str(), rhp.str.c_str()) != 0;
}

bool String::operator< (const String& rhp) const
{
	return str < rhp.str;
}

bool String::operator> (const String& rhp) const
{
	return str > rhp.str;
}

bool String::operator<= (const String& rhp) const
{
	return str <= rhp.str;
}

bool String::operator>= (const String& rhp) const
{
	return str >= rhp.str;
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

