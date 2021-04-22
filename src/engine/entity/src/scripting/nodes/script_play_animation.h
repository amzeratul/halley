#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptPlayAnimation final : public ScriptNodeTypeBase<void> {
	public:
		String getName() const override { return "playAnimation"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		uint8_t getNumTargetPins() const override { return 1; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
