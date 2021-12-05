#include "halley/core/graphics/blend.h"
#include <halley/text/string_converter.h>
#include <halley/bytes/byte_serializer.h>

using namespace Halley;

BlendType::BlendType(BlendMode mode, bool premultiplied)
	: mode(mode)
	, premultiplied(premultiplied)
{}

bool BlendType::operator==(const BlendType& other) const
{
	return mode == other.mode && premultiplied == other.premultiplied;
}

bool BlendType::operator!=(const BlendType& other) const
{
	return mode != other.mode && premultiplied != other.premultiplied;
}

bool BlendType::operator<(const BlendType& other) const
{
	if (mode != other.mode) {
		return mode < other.mode;
	}
	return premultiplied < other.premultiplied;
}

bool BlendType::hasBlend() const
{
	return mode != BlendMode::Undefined && mode != BlendMode::Opaque;
}

void BlendType::serialize(Serializer& s) const
{
	s << mode;
	s << premultiplied;
}

void BlendType::deserialize(Deserializer& s)
{
	s >> mode;
	s >> premultiplied;
}
