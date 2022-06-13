#pragma once

#include <cstdint>

namespace Halley {
 	using ScriptNodeId = uint16_t;
	using ScriptPinId = uint8_t;

	enum class ScriptNodeExecutionState : uint8_t {
		Done,
		Fork,
		ForkAndConvertToWatcher,
		Executing,
		Restart,
		Terminate,
    	MergeAndWait,
		MergeAndContinue
	};

	enum class ScriptNodeClassification : uint8_t {
		FlowControl,
		State,
		Action,
		Variable,
		Terminator, // As in start/end, not as in Arnie
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

	enum class ScriptNodePinDirection : uint8_t {
		Input,
		Output
	};

	enum class ScriptPinSide : uint8_t {
		Undefined,
		Left,
		Right,
		Top,
		Bottom
	};

	struct ScriptNodePinType {
		ScriptNodeElementType type = ScriptNodeElementType::Undefined;
		ScriptNodePinDirection direction = ScriptNodePinDirection::Input;
		bool isCancellable : 1;
		
		ScriptNodePinType(ScriptNodeElementType type = ScriptNodeElementType::Undefined, ScriptNodePinDirection direction = ScriptNodePinDirection::Input, bool cancellable = false)
			: type(type)
			, direction(direction)
			, isCancellable(cancellable)
		{}

		[[nodiscard]] bool operator==(const ScriptNodePinType& other) const
		{
			return type == other.type && direction == other.direction;
		}

		[[nodiscard]] bool operator!=(const ScriptNodePinType& other) const
		{
			return type != other.type || direction != other.direction;
		}

		[[nodiscard]] ScriptPinSide getSide() const
		{
			switch (type) {
			case ScriptNodeElementType::ReadDataPin:
			case ScriptNodeElementType::WriteDataPin:
			case ScriptNodeElementType::FlowPin:
				return direction == ScriptNodePinDirection::Input ? ScriptPinSide::Left : ScriptPinSide::Right;
			case ScriptNodeElementType::TargetPin:
				return direction == ScriptNodePinDirection::Input ? ScriptPinSide::Top : ScriptPinSide::Bottom;
			default:
				return ScriptPinSide::Undefined;
			}
		}

		[[nodiscard]] bool isMultiConnection() const
		{
			return (type == ScriptNodeElementType::ReadDataPin && direction == ScriptNodePinDirection::Output)
				|| (type == ScriptNodeElementType::WriteDataPin && direction == ScriptNodePinDirection::Input)
				|| (type == ScriptNodeElementType::FlowPin)
				|| (type == ScriptNodeElementType::TargetPin && direction == ScriptNodePinDirection::Output);
		}

		[[nodiscard]] bool canConnectTo(const ScriptNodePinType& other) const
		{
			return type == other.type && direction != other.direction;
		}

		[[nodiscard]] ScriptNodePinType getReverseDirection() const
		{
			return ScriptNodePinType(type, direction == ScriptNodePinDirection::Input ? ScriptNodePinDirection::Output : ScriptNodePinDirection::Input);
		}
	};
}
