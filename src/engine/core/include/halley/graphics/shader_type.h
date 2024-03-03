#pragma once

#include <array>
#include "halley/text/string_converter.h"

namespace Halley
{
	enum class ShaderType
	{
		Vertex,
		Pixel,
		Geometry,
		Compute,
		Combined,

		NumOfShaderTypes
	};

	template <>
	struct EnumNames<ShaderType> {
		constexpr std::array<const char*, 5> operator()() const {
			return{{
				"vertex",
				"pixel",
				"geometry",
				"compute",
				"combined"
			}};
		}
	};
}
