#pragma once

#include <cstdint>

namespace Halley {
    enum class ScriptNodeExecutionState {
		Done,
		Executing,
		Restart,
		Terminate
	};

	enum class ScriptNodeClassification {
		Terminator, // As in start/end, not as in Arnie
		FlowControl,
		Variable,
		Action
	};

	enum class ScriptNodeElementType : uint8_t {
		Undefined,
		Node,
		FlowPin,
		DataPin,
		TargetPin
	};

	enum class ScriptNodePinDirection : uint8_t {
		Input,
		Output
	};

	struct ScriptNodePinType {
		ScriptNodeElementType type = ScriptNodeElementType::Undefined;
		ScriptNodePinDirection direction = ScriptNodePinDirection::Input;
	};
}
