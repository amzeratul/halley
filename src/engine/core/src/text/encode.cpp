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

#include <halley/utils/utils.h>
#include "halley/text/encode.h"
#include "halley/support/assert.h"
#include "halley/support/exception.h"
#include "halley/support/logger.h"

using namespace Halley;

namespace {
	const char* base16dict = "0123456789abcdef";
	const char* base64dict = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	const char* base64URLdict = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	Bytes base64reverse;
}

static void initBase64()
{
	static bool hasInit = false;
	if (!hasInit) {
		base64reverse.resize(256, 254);
		for (size_t i = 0; i < 64; i++) {
			base64reverse[base64dict[i]] = char(i);
		}
		for (size_t i = 62; i < 64; i++) {
			base64reverse[base64URLdict[i]] = char(i);
		}
		base64reverse['='] = 255;
		hasInit = true;
	}
}

String Encode::encodeBase16(gsl::span<const gsl::byte> in)
{
	const char characters[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

	String result;
	size_t size = size_t(in.size());
	result.cppStr().resize(size * 2, '0');
	for (size_t i = 0; i < size; ++i) {
		const auto c = reinterpret_cast<const uint8_t*>(in.data())[i];
		result[i * 2] = characters[(c & 0xF0) >> 4];
		result[i * 2 + 1] = characters[c & 0x0F];
	}

	return result;
}

void Encode::encodeBase16(gsl::span<const gsl::byte> in, gsl::span<char> out)
{
    size_t size = in.size();
    Expects(out.size() == 2 * size);

    const gsl::byte* src = in.data();
    char* dst = out.data();

    for (auto i = 0; i < size; i++, src++, dst += 2) {
        auto c = uint8_t(*src);
        dst[0] = base16dict[(c & 0xf0) >> 4];
        dst[1] = base16dict[c & 0x0f];
    }
}

void Encode::decodeBase16(std::string_view in, gsl::span<gsl::byte> bytes)
{
	auto charToVal = [&] (char character) -> uint32_t
	{
		if (character >= '0' && character <= '9') {
			return character - '0';
		} else if (character >= 'A' && character <= 'F') {
			return character - 'A' + 10;
		} else if (character >= 'a' && character <= 'f') {
			return character - 'a' + 10;
		} else {
			throw Exception("Invalid hex string \"" + String(in) + "\".", HalleyExceptions::Utils);
		}
	};
	
	const size_t inSize = in.size();
	const size_t outSize = inSize / 2;
	Expects(inSize % 2 == 0);
	Expects(size_t(bytes.size()) >= outSize);

	for (size_t i = 0; i < outSize; ++i) {
		const auto high = charToVal(in[i * 2]);
		const auto low = charToVal(in[i * 2 + 1]);
		bytes[i] = gsl::byte((high << 4) | low);
	}
}

Bytes Encode::decodeBase16(std::string_view in)
{
	Bytes result;
	result.resize(in.size() / 2);
	decodeBase16(in, result.byte_span());
	return result;
}

String Encode::encodeBase64(gsl::span<const gsl::byte> in, bool url)
{
	size_t sz = in.size();
	Bytes result(((sz+2) / 3) * 4);
	const char* dict = url ? base64URLdict : base64dict;

	for (size_t i=0; i<sz; i+=3) {
		// Input bytes
		int available = std::min(int(sz-i), 3);
		assert (available >= 1);
		unsigned int inByte = (uint8_t(in[i]) << 16) | (available >= 2 ? uint8_t(in[i+1]) << 8 : 0u) | (available >= 3 ? uint8_t(in[i+2]) : 0u);

		// Output bytes
		unsigned int o0 = (inByte >> 18) & 0x3F;
		unsigned int o1 = (inByte >> 12) & 0x3F;
		unsigned int o2 = (inByte >> 6) & 0x3F;
		unsigned int o3 = (inByte >> 0) & 0x3F;

		size_t outPos = i / 3 * 4;
		result[outPos] = dict[o0];
		result[outPos+1] = dict[o1];
		result[outPos+2] = available >= 2 ? dict[o2] : '=';
		result[outPos+3] = available >= 3 ? dict[o3] : '=';
	}

	if (url) {
		while (!result.empty() && result.back() == '=') {
			result.pop_back();
		}
	}

	return String(reinterpret_cast<const char*>(result.data()), result.size());
}

Bytes Encode::decodeBase64(std::string_view in)
{
	Bytes result(getBase64Length(in));
	decodeBase64(in, result.byte_span());
	return result;
}

void Encode::decodeBase64(std::string_view in, gsl::span<gsl::byte> out)
{
	initBase64();

	const auto& reverse = base64reverse.const_span();
	const auto outWrite = gsl::span<uint8_t>(reinterpret_cast<uint8_t*>(out.data()), out.size());

	const auto n = in.length();
	auto getIn = [&](size_t i) -> int
	{
		return i >= n ? 255 : reverse[in[i]];
	};

	for (size_t i = 0; i < n; i += 4) {
		const int b0 = getIn(i);
		const int b1 = getIn(i + 1);
		int b2 = getIn(i + 2);
		int b3 = getIn(i + 3);

		if (b0 == 254 || b1 == 254 || b2 == 254 || b3 == 254) {
			throw Exception("Invalid Base64 encoded string", HalleyExceptions::Utils);
		}

		int available = 3;
		if (b3 == 255) {
			available = 2;
			b3 = 0;
		}
		if (b2 == 255) {
			available = 1;
			b2 = 0;
		}

		unsigned int val = static_cast<unsigned int>((b0 << 18) | (b1 << 12) | (b2 << 6) | (b3));
		size_t outPos = i * 3 / 4;

		outWrite[outPos] = (val >> 16) & 0xFF;
		if (available >= 2) {
			outWrite[outPos + 1] = (val >> 8) & 0xFF;
		}
		if (available == 3) {
			outWrite[outPos + 2] = (val >> 0) & 0xFF;
		}
	}
}

size_t Encode::getBase64Length(std::string_view in)
{
	size_t sz = in.length();
	sz = alignUp<size_t>(sz, 4);
	assert(sz % 4 == 0);

	size_t resLen = sz * 3 / 4;
	if (resLen > 0) {
		if (in.length() <= sz - 2 || in[sz - 2] == '=') {
			resLen -= 2;
		} else if (in.length() <= sz - 1 || in[sz - 1] == '=') {
			resLen -= 1;
		}
	}

	return resLen;
}

static void flushTo(Vector<char>& toFlush, Vector<char>& dest)
{
	if (toFlush.size() > 0) {
		for (size_t i=0; i<toFlush.size(); i += 127) {
			int toWrite = std::min(int(toFlush.size() - i), 127);
			dest.push_back(char(toWrite));
			dest.insert(dest.end(), toFlush.begin()+i, toFlush.begin()+i+toWrite);
		}
	}
	toFlush.clear();
}

Vector<char> Encode::encodeRLE(const Vector<char>& in)
{
	Vector<char> result;
	Vector<char> toFlush;

	size_t totalLen = in.size();
	for (size_t i=0; i<totalLen; ) {
		// Compute run from here
		char byte = in[i];
		int len = 1;
		for (size_t j = i + 1; j < totalLen && in[j] == byte && len < 127; j++, len++);

		// Length of run, only write as RLE if it's at least 3 bytes long
		if (len >= 3) {
			// Flush any waiting to be flushed
			flushTo(toFlush, result);

			// Encode sequence
			result.push_back(char(0x80 | len));
			result.push_back(byte);

			i += len;
		} else {
			// Insert on list of stuff waiting to be flushed
			toFlush.insert(toFlush.end(), in.begin()+i, in.begin()+i+len);
			i++;
		}
	}
	flushTo(toFlush, result);

	return result;
}

Vector<char> Encode::decodeRLE(const Vector<char>& in)
{
	Vector<char> result;
	for (size_t i=0; i<in.size(); ) {
		int control = in[i];
		int len = control & 0x7F;
		if (control & 0x80) {
			// Sequence
			char byte = in[i+1];
			result.insert(result.end(), len, byte);
			i += 2;
		} else {
			// Bunch of stuff
			result.insert(result.end(), in.begin() + i + 1, in.begin() + i + 1 + len);
			i += len + 1;
		}
	}
	return result;
}

String Encode::encodeURL(std::string_view in)
{
	auto isValid = [] (char c)
	{
		return (c >= 'A' && c <= 'Z')
			|| (c >= 'a' && c <= 'z')
			|| (c >= '0' && c <= '9')
			|| (c == '-')
			|| (c == '.')
			|| (c == '_')
			|| (c == '~');
	};

	char buffer[4];
	buffer[0] = '%';
	buffer[3] = 0;

	size_t len = 0;
	for (auto c: in) {
		len += isValid(c) ? 1 : 3;
	}

	std::string result;
	result.reserve(len);

	for (auto c: in) {
		if (isValid(c)) {
			result += c;
		} else {
			const auto b = uint8_t(c);
			buffer[1] = base16dict[(b & 0xf0) >> 4];
			buffer[2] = base16dict[b & 0x0f];
			result += buffer;
		}
	}

	return result;
}

String Encode::readBytesAsUTF8String(gsl::span<const gsl::byte> bytes)
{
	String buffer;
	std::string result;
	auto utf8Src = std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size());

	if (bytes.size() >= 3 && bytes[0] == std::byte{ 0xEF } && bytes[1] == std::byte{ 0xBB } && bytes[2] == std::byte{ 0xBF }) {
		// UTF-8 BOM
		utf8Src = utf8Src.substr(3);
	} else if (bytes.size() >= 2 && bytes[0] == std::byte{ 0xFF } && bytes[1] == std::byte{ 0xFE }) {
		// UTF-16 LE BOM
		const auto wStrView = std::wstring_view(reinterpret_cast<const wchar_t*>(bytes.data()), bytes.size() / 2);
		buffer = String(wStrView);
		utf8Src = buffer;
	} else if (bytes.size() >= 2 && bytes[0] == std::byte{ 0xFE } && bytes[1] == std::byte{ 0xFF }) {
		// UTF-16 BE BOM
		throw Exception("UTF-16 Big Endian is not supported", HalleyExceptions::Utils);
	}

	size_t len = utf8Src.size();
	size_t outLen = 0;
	result.reserve(len);
	for (size_t i = 0; i < len; ++i) {
		const char cur = utf8Src[i];
		const char next = i < len - 1 ? utf8Src[i + 1] : 0;
		if (cur == '\r' && next == '\n') {
			continue;
		} else if (cur == '\r') {
			result += '\n';
			++outLen;
		} else {
			result += cur;
			++outLen;
		}
	}
	result.resize(outLen);

	return String(std::move(result));
}
