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
		Combined,

		NumOfShaderTypes
	};

	template <>
	struct EnumNames<ShaderType> {
		constexpr auto operator()() const {
			return std::to_array({
				"vertex",
				"pixel",
				"geometry",
				"combined"
			});
		}
	};
}
