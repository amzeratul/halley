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
		void doInitData(ScriptForLoopData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
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

	class ScriptLerpLoopData : public ScriptStateData<ScriptLerpLoopData> {
	public:
		float time = 0;

		ScriptLerpLoopData() = default;
		ScriptLerpLoopData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};
	
	class ScriptLerpLoop final : public ScriptNodeTypeBase<ScriptLerpLoopData> {
	public:
		String getId() const override { return "lerpLoop"; }
		String getName() const override { return "Lerp Loop"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/lerp.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		bool canKeepData() const override { return true; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getLabel(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		std::pair<String, Vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, ScriptPinId elementIdx) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const override;

		void doInitData(ScriptLerpLoopData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptLerpLoopData& curData) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptLerpLoopData& curData) const override;
		bool doIsStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptPinId outPin, ScriptLerpLoopData& curData) const override;
	};


	class ScriptEveryFrameData : public ScriptStateData<ScriptEveryFrameData> {
	public:
		int lastFrameN = -1;

		ScriptEveryFrameData() = default;
		ScriptEveryFrameData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};
	
	class ScriptEveryFrame final : public ScriptNodeTypeBase<ScriptEveryFrameData> {
	public:
		String getId() const override { return "everyFrame"; }
		String getName() const override { return "Every Frame"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/every_frame.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		bool canKeepData() const override;

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		std::pair<String, Vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, ScriptPinId elementIdx) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const override;

		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptEveryFrameData& curData) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptEveryFrameData& curData) const override;
		void doInitData(ScriptEveryFrameData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
	};


	class ScriptEveryTimeData : public ScriptStateData<ScriptEveryTimeData> {
	public:
		float time = 0;

		ScriptEveryTimeData() = default;
		ScriptEveryTimeData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};
	
	class ScriptEveryTime final : public ScriptNodeTypeBase<ScriptEveryTimeData> {
	public:
		String getId() const override { return "everyTime"; }
		String getName() const override { return "Every Time"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/every_time.png"; }
		String getLabel(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptEveryTimeData& curData) const override;
		void doInitData(ScriptEveryTimeData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		bool canKeepData() const override;
	};
}
