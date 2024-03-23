#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptStart final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "start"; }
		String getName() const override { return "Start"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/start.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		bool canAdd() const override { return false; }
		bool canDelete() const override { return false; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		String getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
		EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const override;

	private:
		std::optional<std::pair<GraphNodeId, GraphPinId>> getOtherPin(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const;
	};

	class ScriptDestructor final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "destructor"; }
		String getName() const override { return "Destructor"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/destructor.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }

		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptRestart final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "restart"; }
		String getName() const override { return "Restart"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/restart.png"; }
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
	
	class ScriptStop final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "stop"; }
		String getName() const override { return "Stop"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/stop.png"; }
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
	
	class ScriptSpinwait final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "spinwait"; }
		String getName() const override { return "Spinwait"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/spinwait.png"; }
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptStartScriptData final : public ScriptStateData<ScriptStartScriptData> {
	public:
		EntityId target;
		String scriptName;

		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptStartScript final : public ScriptNodeTypeBase<ScriptStartScriptData> {
	public:
		String getId() const override { return "startScript"; }
		String getName() const override { return "Start Script"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/start.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		void updateSettings(BaseGraphNode& node, const BaseGraph& graph, Resources& resources) const override;

		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		bool hasDestructor(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;

		void doInitData(ScriptStartScriptData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptStartScriptData& data) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptStartScriptData& data) const override;
	};
	
	class ScriptStopScript final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "stopScript"; }
		String getName() const override { return "Stop Script"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/stop.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptStopTag final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "stopTag"; }
		String getName() const override { return "Stop Tag"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/stop_tag.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};


	class ScriptWaitUntilEndOfFrameData final : public ScriptStateData<ScriptStartScriptData> {
	public:
		std::optional<int> lastFrame = 0;

		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptWaitUntilEndOfFrame final : public ScriptNodeTypeBase<ScriptWaitUntilEndOfFrameData> {
	public:
		String getId() const override { return "waitUntilEndOfFrame"; }
		String getName() const override { return "Wait Until EOF"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/wait_until_eof.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;

		void doInitData(ScriptWaitUntilEndOfFrameData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptWaitUntilEndOfFrameData& data) const override;
	};
}
