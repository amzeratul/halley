#pragma once

#include "halley/graph/base_graph_enums.h"

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
		Return,
		Detach
	};

	enum class ScriptNodeClassification : uint8_t {
		FlowControl,
		State,
		Action,
		Variable,
		Expression,
		Terminator, // As in start/end, not as in Arnie
		Function,
		NetworkFlow,
		Comment,
		DebugDisplay,
		Unknown
	};

	enum class ScriptNodeElementType : uint8_t {
		Undefined = BaseGraphNodeElementType::Undefined,
		Node = BaseGraphNodeElementType::Node,
		FlowPin,
		ReadDataPin,
		WriteDataPin,
		TargetPin
	};
}
