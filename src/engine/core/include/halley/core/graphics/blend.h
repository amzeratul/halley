#pragma once

#include <array>
#include <halley/text/string_converter.h>

namespace Halley
{
	enum class BlendType {
		Undefined,
		Opaque,
		Alpha,
		AlphaPremultiplied,
		Add,
		Multiply,
		Darken,
		Invert
	};

	template <>
	struct EnumNames<BlendType> {
		constexpr std::array<const char*, 8> operator()() const {
			return{{
				"Undefined",
				"Opaque",
				"Alpha",
				"AlphaPremultiplied",
				"Add",
				"Multiply",
				"Darken",
				"Invert"
			}};
		}
	};
}
