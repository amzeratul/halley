#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptStart final : public ScriptNodeTypeBase<void> {
	public:
		String getName() const override { return "start"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		uint8_t getNumInputPins() const override { return 0; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
