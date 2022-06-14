#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptInputButton final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "scriptInputButton"; }
		String getName() const override { return "Input Button"; }
		String getLabel(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/input_button.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::State; }
		std::pair<String, Vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, ScriptPinId elementIdx) const override;

		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
