#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptWaitData : public ScriptStateData<ScriptWaitData> {
	public:
		float timeLeft = 0;
		bool setFromInput = false;

		ScriptWaitData() = default;
		ScriptWaitData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};
	
	class ScriptWait final : public ScriptNodeTypeBase<ScriptWaitData> {
	public:
		String getId() const override { return "wait"; }
		String getName() const override { return "Wait (Time)"; }
		String getLabel(const BaseGraphNode& node) const override;
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/wait.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }

		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptWaitData& curData) const override;
		void doInitData(ScriptWaitData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
	};
}
