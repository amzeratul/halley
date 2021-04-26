#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptStart final : public ScriptNodeTypeBase<void> {
	public:
		String getName() const override { return "start"; }
		std::pair<String, std::vector<ColourOverride>> getDescription(const ScriptGraphNode& node, const World& world) const override;
		String getIconName() const override { return "script_icons/start.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		uint8_t getNumInputPins() const override { return 0; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
