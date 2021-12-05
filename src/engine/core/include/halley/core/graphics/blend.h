#pragma once

#include <array>
#include "halley/text/enum_names.h"

namespace Halley
{
	class Deserializer;
	class Serializer;

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
		BlendType(BlendMode mode, bool premultiplied = false);

		bool operator==(const BlendType& other) const;
		bool operator!=(const BlendType& other) const;
		bool operator<(const BlendType& other) const;

		bool hasBlend() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};
}
