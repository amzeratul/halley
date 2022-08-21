#pragma once

#include <cstdint>

namespace Halley {
 	using GraphNodeId = uint16_t;
	using GraphPinId = uint8_t;

	enum class GraphNodePinDirection : uint8_t {
		Input,
		Output
	};

	enum class GraphPinSide : uint8_t {
		Undefined,
		Left,
		Right,
		Top,
		Bottom
	};
}
