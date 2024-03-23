#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptFunctionCallExternal final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "callExternal"; }
		String getName() const override { return "Call Function (External)"; }
		String getIconName(const BaseGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Function; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;

		void updateSettings(BaseGraphNode& node, const BaseGraph& graph, Resources& resources) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
		EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const override;

	private:
		size_t getNumOfInputPins(const BaseGraphNode& node) const;
		std::optional<std::pair<GraphNodeId, GraphPinId>> getStartNodePin(ScriptEnvironment& environment, const BaseGraphNode& node, size_t pinN) const;
		std::optional<std::pair<GraphNodeId, GraphPinId>> getReturnNodePin(ScriptEnvironment& environment, const BaseGraphNode& node, size_t pinN) const;
	};

	class ScriptFunctionReturn final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "return"; }
		String getName() const override { return "Return"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/function_return.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
