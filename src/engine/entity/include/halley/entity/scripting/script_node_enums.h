#pragma once

#include "halley/core/graph/base_graph_enums.h"

namespace Halley {
	enum class ScriptNodeExecutionState : uint8_t {
		Done,
		Fork,
		ForkAndConvertToWatcher,
		Executing,
		Restart,
		Terminate,
    	MergeAndWait,
		MergeAndContinue,
		Call,
		Return
	};

	enum class ScriptNodeClassification : uint8_t {
		FlowControl,
		State,
		Action,
		Variable,
		Expression,
		Terminator, // As in start/end, not as in Arnie
		Function,
		Unknown
	};

	enum class ScriptNodeElementType : uint8_t {
		Undefined,
		Node,
		FlowPin,
		ReadDataPin,
		WriteDataPin,
		TargetPin
	};

	struct GraphNodePinType {
		ScriptNodeElementType type = ScriptNodeElementType::Undefined;
		GraphNodePinDirection direction = GraphNodePinDirection::Input;
		bool isCancellable : 1;
		bool forceHorizontal : 1;
		
		GraphNodePinType(ScriptNodeElementType type = ScriptNodeElementType::Undefined, GraphNodePinDirection direction = GraphNodePinDirection::Input, bool cancellable = false, bool forceHorizontal = false)
			: type(type)
			, direction(direction)
			, isCancellable(cancellable)
			, forceHorizontal(forceHorizontal)
		{}

		[[nodiscard]] bool operator==(const GraphNodePinType& other) const
		{
			return type == other.type && direction == other.direction && isCancellable == other.isCancellable && forceHorizontal == other.forceHorizontal;
		}

		[[nodiscard]] bool operator!=(const GraphNodePinType& other) const
		{
			return !(*this == other);
		}

		[[nodiscard]] GraphPinSide getSide() const
		{
			return direction == GraphNodePinDirection::Input ? GraphPinSide::Left : GraphPinSide::Right;
		}

		[[nodiscard]] bool canConnectTo(const GraphNodePinType& other) const
		{
			return type == other.type && direction != other.direction;
		}

		[[nodiscard]] GraphNodePinType getReverseDirection() const
		{
			return GraphNodePinType(type, direction == GraphNodePinDirection::Input ? GraphNodePinDirection::Output : GraphNodePinDirection::Input);
		}
	};
}
