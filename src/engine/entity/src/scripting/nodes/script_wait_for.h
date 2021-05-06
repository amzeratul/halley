#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptWaitFor final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "waitFor"; }
		String getName() const override { return "Wait Util"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/wait_for.png"; }
		gsl::span<const PinType> getPinConfiguration() const override;
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
