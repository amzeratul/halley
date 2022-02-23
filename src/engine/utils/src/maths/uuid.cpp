#include "halley/maths/uuid.h"
#include "halley/text/string_converter.h"
#include "halley/text/encode.h"
#include "halley/maths/random.h"
#include "halley/bytes/byte_serializer.h"
#include <cstring> // needed for memset and memcmp

#include "halley/data_structures/config_node.h"

using namespace Halley;

UUID::UUID()
{
	qwords.fill(0);
}

UUID::UUID(std::array<Byte, 16> b)
{
	memcpy(qwords.data(), b.data(), 16);
}

UUID::UUID(gsl::span<const gsl::byte> b)
{
	if (b.size_bytes() < 16) {
		qwords.fill(0);
	}
	memcpy(qwords.data(), b.data(), std::min(b.size_bytes(), size_t(16)));
}

UUID::UUID(const Bytes& b)
{
	if (b.size() < 16) {
		qwords.fill(0);
	}
	memcpy(qwords.data(), b.data(), std::min(b.size(), size_t(16)));
}

UUID::UUID(const String& str)
{
	if (str.length() != 36) {
		throw Exception("Invalid UUID format", HalleyExceptions::Utils);
	}
	const auto span = getWriteableBytes();
	const std::string_view strView = str;
	Encode::decodeBase16(strView.substr(0, 8), span.subspan(0, 4));
	Encode::decodeBase16(strView.substr(9, 4), span.subspan(4, 2));
	Encode::decodeBase16(strView.substr(14, 4), span.subspan(6, 2));
	Encode::decodeBase16(strView.substr(19, 4), span.subspan(8, 2));
	Encode::decodeBase16(strView.substr(24, 12), span.subspan(10, 6));
}

UUID::UUID(const ConfigNode& node)
	: UUID(node.asString())
{
}

bool UUID::operator==(const UUID& other) const
{
	return qwords == other.qwords;
}

bool UUID::operator!=(const UUID& other) const
{
	return qwords != other.qwords;
}

bool UUID::operator<(const UUID& other) const
{
	return qwords < other.qwords;
}

String UUID::toString() const
{
	using namespace Encode;
	const auto span = getBytes();
	return encodeBase16(span.subspan(0, 4)) + "-"
 		 + encodeBase16(span.subspan(4, 2)) + "-"
		 + encodeBase16(span.subspan(6, 2)) + "-"
		 + encodeBase16(span.subspan(8, 2)) + "-"
		 + encodeBase16(span.subspan(10, 6));
}

ConfigNode UUID::toConfigNode() const
{
	return ConfigNode(toString());
}

UUID UUID::generate()
{
	UUID result;
	auto bs = result.getWriteableBytes();
	auto bytes = gsl::span<uint8_t>(reinterpret_cast<uint8_t*>(bs.data()), 16);
	Random::getGlobal().getBytes(bs);
	bytes[6] = (bytes[6] & 0b00001111) | (4 << 4); // Version 4
	bytes[8] = (bytes[8] & 0b00111111) | (0b10 << 6); // Variant 1
	return result;
}

UUID UUID::generateFromUUIDs(const UUID& one, const UUID& two)
{
	const auto oneBytes = one.getBytes();
	const auto twoBytes = two.getBytes();

	Expects(oneBytes.size() == twoBytes.size());
	
	UUID result;
	auto bs = result.getWriteableBytes();
	auto bytes = gsl::span<uint8_t>(reinterpret_cast<uint8_t*>(bs.data()), 16);
	for (auto i = 0; i < oneBytes.size(); i++) {
		bytes[i] = Byte(oneBytes[i] ^ twoBytes[i]);
	}
	bytes[6] = (bytes[6] & 0b00001111) | (4 << 4); // Version 4
	bytes[8] = (bytes[8] & 0b00111111) | (0b10 << 6); // Variant 1
	return result;
}

bool UUID::isValid() const
{
	for (size_t i = 0; i < qwords.size(); ++i) {
		if (qwords[i] != 0) {
			return true;
		}
	}
	return false;
}

gsl::span<const gsl::byte> UUID::getBytes() const
{
	return gsl::as_bytes(gsl::span<const uint64_t>(qwords));
}

gsl::span<gsl::byte> UUID::getWriteableBytes()
{
	return gsl::as_writable_bytes(gsl::span<uint64_t>(qwords));
}

gsl::span<const uint64_t> UUID::getUint64Bytes() const
{
	return qwords;
}

void UUID::serialize(Serializer& s) const
{
	s << getBytes();
}

void UUID::deserialize(Deserializer& s)
{
	s >> getWriteableBytes();
}
