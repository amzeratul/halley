#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptWaitFor final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "waitFor"; }
		String getName() const override { return "Wait (Condition)"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/wait_for.png"; }
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
