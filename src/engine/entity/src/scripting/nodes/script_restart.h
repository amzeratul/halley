#pragma once
#include "scripting/script_environment.h"

namespace Halley {	
	class ScriptRestart final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "restart"; }
		String getName() const override { return "Restart"; }
		String getIconName() const override { return "script_icons/restart.png"; }
		uint8_t getNumFlowOutputPins() const override { return 0; }
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
