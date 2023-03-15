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

using namespace Halley;

static const char* base64dict = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static Bytes base64reverse;

static void initBase64()
{
	static bool hasInit = false;
	if (!hasInit) {
		base64reverse.resize(256);
		for (size_t i=0; i<64; i++) {
			base64reverse[base64dict[i]] = char(i);
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

String Encode::encodeBase64(const Bytes& in)
{
	size_t sz = in.size();
	Bytes result(((sz+2) / 3) * 4);

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
		result[outPos] = base64dict[o0];
		result[outPos+1] = base64dict[o1];
		result[outPos+2] = available >= 2 ? base64dict[o2] : '=';
		result[outPos+3] = available >= 3 ? base64dict[o3] : '=';
	}

	return String(reinterpret_cast<const char*>(result.data()), result.size());
}

Bytes Encode::decodeBase64(const String& in)
{
	initBase64();

	size_t sz = in.length();
	assert(sz % 4 == 0);
	size_t resLen = sz * 3 / 4;
	if (in[sz-2] == '=') resLen -= 2;
	else if (in[sz-1] == '=') resLen -= 1;
	Bytes result(resLen);

	for (size_t i=0; i<sz; i+=4) {
		int b0 = base64reverse[in[i]];
		int b1 = base64reverse[in[i+1]];
		int b2 = base64reverse[in[i+2]];
		int b3 = base64reverse[in[i+3]];

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

		result[outPos] = (val >> 16) & 0xFF;
		if (available >= 2) result[outPos+1] = (val >> 8) & 0xFF;
		if (available == 3) result[outPos+2] = (val >> 0) & 0xFF;
	}

	return result;
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
