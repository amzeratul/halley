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

UUID::UUID(gsl::span<const std::byte> b)
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

UUID::UUID(std::string_view strView)
{
	if (strView.length() != 36) {
		throw Exception("Invalid UUID format", HalleyExceptions::Utils);
	}
	const auto span = getWriteableBytes();
	Encode::decodeBase16(strView.substr(0, 8), span.subspan(0, 4));
	Encode::decodeBase16(strView.substr(9, 4), span.subspan(4, 2));
	Encode::decodeBase16(strView.substr(14, 4), span.subspan(6, 2));
	Encode::decodeBase16(strView.substr(19, 4), span.subspan(8, 2));
	Encode::decodeBase16(strView.substr(24, 12), span.subspan(10, 6));
}

UUID::UUID(const ConfigNode& node)
	: UUID()
{
	if (node.getType() == ConfigNodeType::String) {
		if (auto value = tryParse(node.asString())) {
			*this = *value;
		}
	} else if (node.getType() == ConfigNodeType::Bytes) {
		*this = UUID(node.asBytes());
	}
}

bool UUID::isUUID(std::string_view strView)
{
	if (strView.length() != 36) {
		return false;
	}
	if (strView[8] != '-' || strView[13] != '-' || strView[18] != '-' || strView[23] != '-') {
		return false;
	}
	for (auto c: strView) {
		if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F') && c != '-') {
			return false;
		}
	}
	return true;
}

std::optional<UUID> UUID::tryParse(std::string_view strView)
{
	if (!isUUID(strView)) {
		return {};
	}
	return UUID(strView);
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

UUID UUID::operator^(const UUID& other) const
{
	return xorUUIDs(*this, other);
}

String UUID::toString() const
{
	using namespace Encode;
	const auto span = getBytes();
    String result("........-....-....-....-............");
    auto resultSpan = result.asSpan();
    encodeBase16(span.subspan(0, 4), resultSpan.subspan(0, 8));
    encodeBase16(span.subspan(4, 2), resultSpan.subspan(9, 4));
    encodeBase16(span.subspan(6, 2), resultSpan.subspan(14, 4));
    encodeBase16(span.subspan(8, 2), resultSpan.subspan(19, 4));
    encodeBase16(span.subspan(10, 6), resultSpan.subspan(24, 12));
    return result;
}

ConfigNode UUID::toConfigNode() const
{
	return ConfigNode(toString());
}

UUID UUID::generate()
{
	UUID result;
	auto bs = result.getWriteableBytes();
	Random::getGlobal().getBytes(bs);
	result.setVersionBits();
	return result;
}

UUID UUID::xorUUIDs(const UUID& one, const UUID& two)
{
	UUID result;
	for (size_t i = 0; i < result.bytes.size(); i++) {
		result.bytes[i] = static_cast<uint8_t>(one.bytes[i] ^ two.bytes[i]);
	}
	result.setVersionBits();
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

gsl::span<const std::byte> UUID::getBytes() const
{
	return gsl::as_bytes(gsl::span<const uint64_t>(qwords));
}

gsl::span<std::byte> UUID::getWriteableBytes()
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

void UUID::setVersionBits()
{
	bytes[6] = (bytes[6] & 0b00001111) | (4 << 4); // Version 4
	bytes[8] = (bytes[8] & 0b00111111) | (0b10 << 6); // Variant 1
}

ConfigNode ConfigNodeSerializer<UUID>::serialize(UUID id, const EntitySerializationContext& context)
{
	auto bytes = id.getBytes();
	Bytes result;
	result.resize(bytes.size());
	memcpy(result.data(), bytes.data(), bytes.size());
	return ConfigNode(result);
}

UUID ConfigNodeSerializer<UUID>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return UUID(node.asBytes());
}
