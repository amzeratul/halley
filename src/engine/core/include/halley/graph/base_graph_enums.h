#pragma once

#include <cstdint>

namespace Halley {
 	using GraphNodeId = uint16_t;
	using GraphPinId = uint8_t;
	using GraphElementType = uint8_t;

	enum class BaseGraphNodeElementType : uint8_t {
		Undefined,
		Node
	};

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

	struct GraphNodePinType {
		GraphElementType type = 0;
		GraphNodePinDirection direction = GraphNodePinDirection::Input;
		bool isCancellable : 1;
		bool forceHorizontal : 1;
		bool isDetached : 1;
		bool isNetwork : 1;
		
		GraphNodePinType(GraphElementType type = 0, GraphNodePinDirection direction = GraphNodePinDirection::Input, bool cancellable = false, bool forceHorizontal = false, bool isDetached = false, bool isNetwork = false)
			: type(type)
			, direction(direction)
			, isCancellable(cancellable)
			, forceHorizontal(forceHorizontal)
			, isDetached(isDetached)
			, isNetwork(isNetwork)
		{}

		template <typename T>
		GraphNodePinType(T type = T(0), GraphNodePinDirection direction = GraphNodePinDirection::Input, bool cancellable = false, bool forceHorizontal = false, bool isDetached = false, bool isNetwork = false)
			: type(static_cast<GraphElementType>(type))
			, direction(direction)
			, isCancellable(cancellable)
			, forceHorizontal(forceHorizontal)
			, isDetached(isDetached)
			, isNetwork(isNetwork)
		{}

		[[nodiscard]] bool operator==(const GraphNodePinType& other) const
		{
			return type == other.type && direction == other.direction && isCancellable == other.isCancellable && forceHorizontal == other.forceHorizontal && isDetached == other.isDetached;
		}

		[[nodiscard]] bool operator!=(const GraphNodePinType& other) const
		{
			return !(*this == other);
		}

		[[nodiscard]] GraphPinSide getSide() const
		{
			return direction == GraphNodePinDirection::Input ? GraphPinSide::Left : GraphPinSide::Right;
		}

		[[nodiscard]] GraphNodePinType getReverseDirection() const
		{
			return GraphNodePinType(type, direction == GraphNodePinDirection::Input ? GraphNodePinDirection::Output : GraphNodePinDirection::Input);
		}
	};
}
