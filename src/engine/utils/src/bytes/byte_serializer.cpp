#include <cstring>
#include <string>
#include "halley/bytes/byte_serializer.h"
#include "halley/text/halleystring.h"

using namespace Halley;

SerializerState* ByteSerializationBase::setState(SerializerState* s)
{
	const auto oldState = state;
	state = s;
	return oldState;
}

Serializer::Serializer(SerializerOptions options)
	: ByteSerializationBase(std::move(options))
	, dryRun(true)
{}

Serializer::Serializer(gsl::span<gsl::byte> dst, SerializerOptions options)
	: ByteSerializationBase(std::move(options))
	, dst(dst)
	, dryRun(false)
{}

Serializer& Serializer::operator<<(const std::string& str)
{
	return *this << String(str);
}

Serializer& Serializer::operator<<(const String& str)
{
	if (options.version == 0) {
		const uint32_t sz = static_cast<uint32_t>(str.size());
		*this << sz;
		*this << gsl::as_bytes(gsl::span<const char>(str.c_str(), sz));
	} else {
		if (options.stringToIndex) {
			auto idx = options.stringToIndex(str);
			if (idx) {
				// Found, store index with bit 0 set to 1
				const uint64_t value = uint64_t(options.exhaustiveDictionary ? idx.value() : (size_t(1) | (idx.value() << 1)));
				*this << value;
			} else {
				if (options.exhaustiveDictionary) {
					throw Exception("String \"" + str + "\" not found in serialization dictionary, but it's marked as exhaustive.", HalleyExceptions::Utils);
				}
				
				// Not found, store it with bit 0 set to 0
				const uint64_t sz = uint64_t(str.size());
				*this << (sz << 1);
				*this << gsl::as_bytes(gsl::span<const char>(str.c_str(), sz));
			}
		} else {
			// No dictionary, just store it old style
			const uint64_t sz = str.size();
			*this << sz;
			*this << gsl::as_bytes(gsl::span<const char>(str.c_str(), sz));
		}
	}
	return *this;
}

Serializer& Serializer::operator<<(const Path& path)
{
	return (*this << path.string());
}

Serializer& Serializer::operator<<(gsl::span<const gsl::byte> span)
{
	if (!dryRun) {
		memcpy(dst.data() + size, span.data(), span.size_bytes());
	}
	size += span.size_bytes();
	return *this;
}

Serializer& Serializer::operator<<(const Bytes& bytes)
{
	const uint32_t byteSize = static_cast<uint32_t>(bytes.size());
	*this << byteSize;

	if (!dryRun) {
		memcpy(dst.data() + size, bytes.data(), bytes.size());
	}
	size += bytes.size();
	return *this;
}

void Serializer::serializeVariableInteger(uint64_t val, OptionalLite<bool> sign)
{
	// 7  0sxxxxxx
	// 14 10sxxxxx xxxxxxxx
	// 21 110sxxxx xxxxxxxx xxxxxxxx
	// 28 1110sxxx xxxxxxxx xxxxxxxx xxxxxxxx
	// 35 11110sxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	// 42 111110sx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	// 49 1111110s xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	// 56 11111110 sxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	// 64 11111111 sxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	
	const size_t nBits = size_t(fastLog2Ceil(val)) + (sign ? 1 : 0);
	const size_t nBytes = std::min((nBits - 1) / 7, size_t(8)) + 1; // Total length of this sequence
	std::array<uint8_t, 9> buffer;
	buffer.fill(0);

	// Combine sign into value
	uint64_t toWrite = val;
	if (sign) {
		const size_t signPos = nBytes * 7 + (nBytes == 9 ? 0 : -1); // 9-byte version places it on pos 63, not 62
		toWrite |= uint64_t(sign.value() ? 1 : 0) << signPos;
	}

	// Write header
	// To generate the mask, we get 9 - nBytes to see how many zeroes we need (8 at 1 byte, 7 at 2 bytes, etc), generate that many "1"s, then xor that with 255 (0b11111111) to flip those bits
	const size_t headerBits = std::min(nBytes, size_t(7));
	buffer[0] = uint8_t(255) ^ (uint8_t((1 << (9 - headerBits)) - 1));

	// Write bits
	size_t bitsAvailableOnByte = 8 - headerBits;
	size_t bitsToWrite = nBits;
	size_t curPos = 0;

	while (bitsToWrite > 0) {
		const size_t nBits = std::min(bitsToWrite, bitsAvailableOnByte);
		const uint64_t mask = (uint64_t(1) << bitsAvailableOnByte) - 1;

		buffer[curPos] |= toWrite & mask;

		toWrite >>= nBits;
		bitsAvailableOnByte = 8;
		curPos++;
		bitsToWrite -= nBits;
	}

	*this << gsl::as_bytes(gsl::span<const uint8_t>(buffer.data(), curPos));
}

Deserializer::Deserializer(gsl::span<const gsl::byte> src, SerializerOptions options)
	: ByteSerializationBase(std::move(options))
	, src(src)
{
}

Deserializer::Deserializer(const Bytes& src, SerializerOptions options)
	: ByteSerializationBase(std::move(options))
	, src(gsl::as_bytes(gsl::span<const Halley::Byte>(src)))
{
}

Deserializer& Deserializer::operator>>(std::string& str)
{
	String s;
	*this >> s;
	str = s.cppStr();
	return *this;
}

Deserializer& Deserializer::operator>>(String& str)
{
	auto readRawString = [&] (size_t size)
	{
		ensureSufficientBytesRemaining(size);
		str = String(reinterpret_cast<const char*>(src.data() + pos), size);
		pos += size;
	};
	
	if (options.version == 0) {
		uint32_t size;
		*this >> size;
		readRawString(size);
	} else {
		uint64_t value;
		*this >> value;
		
		if (options.indexToString) {
			if (options.exhaustiveDictionary || (value & 0x1) != 0) {
				// Indexed string
				int shift = options.exhaustiveDictionary ? 0 : 1;
				str = options.indexToString(value >> shift);
			} else {
				// Not indexed
				readRawString(value >> 1);
			}
		} else {
			// No dictionary
			readRawString(value);
		}
	}

	return *this;
}

Deserializer& Deserializer::operator>>(Path& p)
{
	std::string s;
	*this >> s;
	p = s;
	return *this;
}

Deserializer& Deserializer::operator>>(gsl::span<gsl::byte> span)
{
	if (span.empty()) {
		return *this;
	}
	Expects(span.size_bytes() > 0);

	ensureSufficientBytesRemaining(size_t(span.size_bytes()));

	memcpy(span.data(), src.data() + pos, span.size_bytes());
	pos += span.size_bytes();
	return *this;
}

Deserializer& Deserializer::operator>>(Bytes& bytes)
{
	uint32_t sz;
	*this >> sz;
	ensureSufficientBytesRemaining(sz);
	bytes.resize(sz);

	auto dst = gsl::as_writable_bytes(gsl::span<Byte>(bytes));
	*this >> dst;
	return *this;
}

void Deserializer::deserializeVariableInteger(uint64_t& val, bool& sign, bool isSigned)
{
	// 7  0sxxxxxx
	// 14 10sxxxxx xxxxxxxx
	// 21 110sxxxx xxxxxxxx xxxxxxxx
	// 28 1110sxxx xxxxxxxx xxxxxxxx xxxxxxxx
	// 35 11110sxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	// 42 111110sx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	// 49 1111110s xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	// 56 11111110 sxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	// 64 11111111 sxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx

	// Read header
	uint8_t header;
	*this >> header;

	// Figure out which pattern we're dealing with
	size_t nBytes = 0;
	if ((header & 0x80) != 0x80) {
		nBytes = 1;
	} else if ((header & 0xC0) != 0xC0) {
		nBytes = 2;
	} else if ((header & 0xE0) != 0xE0) {
		nBytes = 3;
	} else if ((header & 0xF0) != 0xF0) {
		nBytes = 4;
	} else if ((header & 0xF8) != 0xF8) {
		nBytes = 5;
	} else if ((header & 0xFC) != 0xFC) {
		nBytes = 6;
	} else if ((header & 0xFE) != 0xFE) {
		nBytes = 7;
	} else if ((header & 0xFF) != 0xFF) {
		nBytes = 8;
	} else {
		nBytes = 9;
	}
	const size_t headerBits = std::min(nBytes, size_t(7));

	// Read rest of the data
	std::array<uint8_t, 9> buffer;
	buffer[0] = header;
	if (nBytes > 1) {
		*this >> gsl::as_writable_bytes(gsl::span<uint8_t>(buffer.data(), nBytes - 1));
	}

	// Convert to uint64_t
	size_t bitsAvailableOnByte = 8 - headerBits;
	size_t bitsRead = 0;
	uint64_t value = 0;
	for (size_t i = 0; i < nBytes; ++i) {
		const uint64_t byteMask = (uint64_t(1) << bitsAvailableOnByte) - 1;
		value |= (uint64_t(buffer[i]) & byteMask) << bitsRead;
		bitsRead += bitsAvailableOnByte;
		bitsAvailableOnByte = 8;
	}

	// Restore sign
	if (isSigned) {
		const size_t signPos = nBytes * 7 + (nBytes == 9 ? 0 : -1); // 9-byte version places it on pos 63, not 62
		const uint64_t signMask = uint64_t(1) << signPos;
		sign = (value & signMask) != 0;
		value &= ~signMask;
	} else {
		sign = false;
	}

	// Output value
	val = value;
}

void Deserializer::ensureSufficientBytesRemaining(size_t bytes)
{
	if (bytes > getBytesRemaining()) {
		throw Exception("Attempt to deserialize out of bounds", HalleyExceptions::File);
	}
}

size_t Deserializer::getBytesRemaining() const
{
	if (pos > size_t(src.size_bytes())) {
		return 0;
	} else {
		return size_t(src.size_bytes()) - pos;
	}
}
