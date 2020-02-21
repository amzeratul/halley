#include "halley/maths/uuid.h"
#include "halley/text/string_converter.h"
#include "halley/text/encode.h"
#include "halley/maths/random.h"

using namespace Halley;

UUID::UUID()
{
	memset(bytes.data(), 0, 16);
}

UUID::UUID(std::array<Byte, 16> b)
{
	memcpy(bytes.data(), b.data(), 16);
}

UUID::UUID(const String& str)
{
	if (str.length() != 36) {
		throw Exception("Invalid UUID format", HalleyExceptions::Utils);
	}
	const auto span = gsl::span<Byte, 16>(bytes.data(), 16);
	Encode::decodeBase16(str.substr(0, 8), span.subspan(0, 4));
	Encode::decodeBase16(str.substr(9, 4), span.subspan(4, 2));
	Encode::decodeBase16(str.substr(14, 4), span.subspan(6, 2));
	Encode::decodeBase16(str.substr(19, 4), span.subspan(8, 2));
	Encode::decodeBase16(str.substr(24, 12), span.subspan(10, 6));
}

bool UUID::operator==(const UUID& other) const
{
	return memcmp(bytes.data(), other.bytes.data(), size_t(bytes.size())) == 0;
}

bool UUID::operator!=(const UUID& other) const
{
	return memcmp(bytes.data(), other.bytes.data(), size_t(bytes.size())) != 0;
}

String UUID::toString() const
{
	using namespace Encode;
	const auto span = gsl::span<const Byte, 16>(bytes.data(), 16);
	return encodeBase16(span.subspan(0, 4)) + "-"
 		 + encodeBase16(span.subspan(4, 2)) + "-"
		 + encodeBase16(span.subspan(6, 2)) + "-"
		 + encodeBase16(span.subspan(8, 2)) + "-"
		 + encodeBase16(span.subspan(10, 6));
}

UUID UUID::generate()
{
	UUID result;
	auto& bs = result.bytes;
	Random::getGlobal().getBytes(gsl::span<Byte>(bs.data(), bs.size()));
	bs[6] = (bs[6] & 0b00001111) | (4 << 4); // Version 4
	bs[8] = (bs[8] & 0b00111111) | (0b10 << 6); // Variant 1
	return result;
}
