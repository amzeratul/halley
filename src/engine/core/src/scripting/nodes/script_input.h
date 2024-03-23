#pragma once
#include "halley/scripting/script_environment.h"
#include "halley/input/input_exclusive.h"

namespace Halley {
	class ScriptInputButtonData final : public ScriptStateData<ScriptInputButtonData> {
	public:
		uint8_t outputMask = 0;
		std::unique_ptr<InputExclusiveButton> input;
		int lastFrame = -1;

		ScriptInputButtonData() = default;
		ScriptInputButtonData(const ScriptInputButtonData& other);
		ScriptInputButtonData(ScriptInputButtonData&& other) = default;
		ScriptInputButtonData& operator=(const ScriptInputButtonData& other);
		ScriptInputButtonData& operator=(ScriptInputButtonData&& other) = default;

		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
		void finishData() override;
	};

	class ScriptInputButton final : public ScriptNodeTypeBase<ScriptInputButtonData> {
	public:
		String getId() const override { return "inputButton"; }
		String getName() const override { return "Input Button"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/input_button.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::State; }

		String getLabel(const BaseGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		bool hasDestructor(const ScriptGraphNode& node) const override { return true; }
		bool showDestructor() const override { return false; }

		void doInitData(ScriptInputButtonData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptInputButtonData& data) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptInputButtonData& curData) const override;
	};

	class ScriptHasInputLabel final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "hasInputLabel"; }
		String getName() const override { return "Has Input Label"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/input_button.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
}
