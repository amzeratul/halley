#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptSetPosition final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setPosition"; }
		String getName() const override { return "Set Position"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/set_position.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptGetPosition final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "getPosition"; }
		String getName() const override { return "Get Position"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/get_position.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const override;
		
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
}
