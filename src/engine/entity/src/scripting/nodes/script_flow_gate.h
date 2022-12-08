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
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		void doInitData(ScriptFlowGateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowGateData& data) const override;
	};


	class ScriptFlowOnceData : public ScriptStateData<ScriptFlowOnceData> {
	public:
		ScriptFlowOnceData() = default;
		ScriptFlowOnceData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;

		std::optional<bool> active{};
	};

	class ScriptFlowOnce : public ScriptNodeTypeBase<ScriptFlowOnceData> {
	public:
		String getId() const override { return "flowOnce"; }
		String getName() const override { return "Flow Once"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/flow_once.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }

		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		void doInitData(ScriptFlowOnceData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowOnceData& data) const override;
	};


	class ScriptLatchData final : public ScriptStateData<ScriptLatchData> {
	public:
		bool latched = false;
		ConfigNode value;

		ScriptLatchData() = default;
		ScriptLatchData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptLatch final : public ScriptNodeTypeBase<ScriptLatchData> {
	public:
		String getId() const override { return "latch"; }
		String getName() const override { return "Latch"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/latch.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		void doInitData(ScriptLatchData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptLatchData& data) const override;
		void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, ScriptLatchData& curData) const override;
	};
}
