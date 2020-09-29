#pragma once

#include <array>
#include <halley/text/string_converter.h>
#include <halley/bytes/byte_serializer.h>

namespace Halley
{
	enum class BlendMode {
		Undefined,
		Opaque,
		Alpha,
		Add,
		Multiply,
		Darken,
		Invert,
		Max,
		Min
	};

	template <>
	struct EnumNames<BlendMode> {
		constexpr std::array<const char*, 13> operator()() const {
			return{{
				"Undefined",
				"Opaque",
				"Alpha",
				"Add",
				"Multiply",
				"Darken",
				"Invert",
				"Max",
				"Min",
			}};
		}
	};

	class BlendType {
	public:
		BlendMode mode = BlendMode::Undefined;
		bool premultiplied = false;

		BlendType() = default;
		BlendType(BlendMode mode, bool premultiplied = false) : mode(mode), premultiplied(premultiplied) {}

		bool operator==(const BlendType& other) const
		{
			return mode == other.mode && premultiplied == other.premultiplied;
		}

		bool operator!=(const BlendType& other) const
		{
			return mode != other.mode && premultiplied != other.premultiplied;
		}

		bool operator<(const BlendType& other) const
		{
			if (mode != other.mode) {
				return mode < other.mode;
			}
			return premultiplied < other.premultiplied;
		}

		bool hasBlend() const
		{
			return mode != BlendMode::Undefined && mode != BlendMode::Opaque;
		}

		void serialize(Serializer& s) const
		{
			s << mode;
			s << premultiplied;
		}

		void deserialize(Deserializer& s)
		{
			s >> mode;
			s >> premultiplied;
		}
	};
}
