#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class UIWidget;

	class ScriptUIModalData final : public ScriptStateData<ScriptUIModalData> {
	public:
		std::shared_ptr<UIWidget> ui;
		ConfigNode result;

		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptUIModal final : public ScriptNodeTypeBase<ScriptUIModalData> {
	public:
		String getId() const override { return "uiModal"; }
		String getName() const override { return "UI (Modal)"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/ui_modal.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		bool hasDestructor(const ScriptGraphNode& node) const override { return true; }
		bool canKeepData() const override { return true; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;

		void doInitData(ScriptUIModalData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
        Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptUIModalData& data) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptUIModalData& curData) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptUIModalData& curData) const override;
	};

	class ScriptUIInWorldData final : public ScriptStateData<ScriptUIInWorldData> {
	public:
		std::shared_ptr<UIWidget> ui;

		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptUIInWorld final : public ScriptNodeTypeBase<ScriptUIInWorldData> {
	public:
		String getId() const override { return "uiInWorld"; }
		String getName() const override { return "UI (In World)"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/ui_in_world.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		bool hasDestructor(const ScriptGraphNode& node) const override { return true; }
		bool canKeepData() const override { return true; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;

		void doInitData(ScriptUIInWorldData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
        Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptUIInWorldData& data) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptUIInWorldData& curData) const override;
	};
}
