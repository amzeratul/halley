#pragma once

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

		NumberOfBlendTypes
	};
}
