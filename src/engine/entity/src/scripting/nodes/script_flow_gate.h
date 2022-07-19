#pragma once
#include "scripting/script_environment.h"

namespace Halley {

	class ScriptFlowGateData : public ScriptStateData<ScriptFlowGateData> {
	public:
		ScriptFlowGateData() = default;
		ScriptFlowGateData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;

		std::optional<bool> flowing;
	};

	class ScriptFlowGate : public ScriptNodeTypeBase<ScriptFlowGateData> {
	public:
		String getId() const override { return "flowGate"; }
		String getName() const override { return "Flow Gate"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/flow_gate.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::State; }
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		std::pair<String, Vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, ScriptPinId elementIdx) const override;

		void doInitData(ScriptFlowGateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowGateData& data) const override;
	};
}
