#include "halley/maths/uuid.h"
#include "halley/text/string_converter.h"
#include "halley/text/encode.h"
#include "halley/maths/random.h"
#include "halley/bytes/byte_serializer.h"
#include <cstring> // needed for memset and memcmp

using namespace Halley;

UUID::UUID()
{
	memset(bytes.data(), 0, 16);
}

UUID::UUID(std::array<Byte, 16> b)
{
	memcpy(bytes.data(), b.data(), 16);
}

UUID::UUID(gsl::span<const gsl::byte> b)
{
	if (b.size_bytes() < 16) {
		bytes.fill(0);
	}
	memcpy(bytes.data(), b.data(), std::min(b.size_bytes(), size_t(16)));
}

UUID::UUID(const Bytes& b)
{
	if (b.size() < 16) {
		bytes.fill(0);
	}
	memcpy(bytes.data(), b.data(), std::min(b.size(), size_t(16)));
}

UUID::UUID(const String& str)
{
	if (str.length() != 36) {
		throw Exception("Invalid UUID format", HalleyExceptions::Utils);
	}
	const auto span = gsl::span<Byte, 16>(bytes.data(), 16);
	const std::string_view strView = str;
	Encode::decodeBase16(strView.substr(0, 8), span.subspan(0, 4));
	Encode::decodeBase16(strView.substr(9, 4), span.subspan(4, 2));
	Encode::decodeBase16(strView.substr(14, 4), span.subspan(6, 2));
	Encode::decodeBase16(strView.substr(19, 4), span.subspan(8, 2));
	Encode::decodeBase16(strView.substr(24, 12), span.subspan(10, 6));
}

bool UUID::operator==(const UUID& other) const
{
	return memcmp(bytes.data(), other.bytes.data(), size_t(bytes.size())) == 0;
}

bool UUID::operator!=(const UUID& other) const
{
	return memcmp(bytes.data(), other.bytes.data(), size_t(bytes.size())) != 0;
}

bool UUID::operator<(const UUID& other) const
{
	return memcmp(bytes.data(), other.bytes.data(), size_t(bytes.size())) == -1;
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

UUID UUID::generateFromUUIDs(const UUID& one, const UUID& two)
{
	const auto oneBytes = one.getBytes();
	const auto twoBytes = two.getBytes();

	Expects(oneBytes.size() == twoBytes.size());
	
	UUID result;
	auto& bs = result.bytes;
	for (auto i = 0; i < oneBytes.size(); i++) {
		bs[i] = Byte(oneBytes[i] ^ twoBytes[i]);
	}
	bs[6] = (bs[6] & 0b00001111) | (4 << 4); // Version 4
	bs[8] = (bs[8] & 0b00111111) | (0b10 << 6); // Variant 1
	return result;
}

bool UUID::isValid() const
{
	for (size_t i = 0; i < 16; ++i) {
		if (bytes[i] != 0) {
			return true;
		}
	}
	return false;
}

gsl::span<const gsl::byte> UUID::getBytes() const
{
	return gsl::as_bytes(gsl::span<const Byte>(bytes));
}

gsl::span<gsl::byte> UUID::getBytes()
{
	return gsl::as_writable_bytes(gsl::span<Byte>(bytes));
}

void UUID::serialize(Serializer& s) const
{
	s << getBytes();
}

void UUID::deserialize(Deserializer& s)
{
	s >> getBytes();
}
