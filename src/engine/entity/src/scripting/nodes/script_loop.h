#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptForLoopData : public ScriptStateData<ScriptForLoopData> {
	public:
		int iterations = 0;

		ScriptForLoopData() = default;
		ScriptForLoopData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};
	
	class ScriptForLoop final : public ScriptNodeTypeBase<ScriptForLoopData> {
	public:
		String getId() const override { return "for"; }
		String getName() const override { return "For Loop"; }
		String getLabel(const ScriptGraphNode& node) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/loop.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		std::pair<String, Vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, ScriptPinId elementIdx) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptForLoopData& curData) const override;
		void doInitData(ScriptForLoopData& data, const ScriptGraphNode& node, const ConfigNode& nodeData) const override;
		bool doIsStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptPinId outPin, ScriptForLoopData& curData) const override;
		bool canKeepData() const override;
	};

	class ScriptWhileLoop final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "while"; }
		String getName() const override { return "While Loop"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/loop.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		std::pair<String, Vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, ScriptPinId elementIdx) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
		bool doIsStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptPinId outPin) const override;
	};
}
