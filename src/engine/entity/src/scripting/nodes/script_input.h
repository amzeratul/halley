#pragma once
#include "scripting/script_environment.h"
#include "halley/core/input/input_exclusive.h"

namespace Halley {
	class ScriptInputButtonData final : public ScriptStateData<ScriptInputButtonData> {
	public:
		uint8_t outputMask = 0;
		std::unique_ptr<InputExclusiveButton> input;

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
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/input_button.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::State; }

		String getLabel(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		std::pair<String, Vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, ScriptPinId elementIdx) const override;

		bool hasDestructor(const ScriptGraphNode& node) const override { return true; }
		bool showDestructor() const override { return false; }

		void doInitData(ScriptInputButtonData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptInputButtonData& data) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptInputButtonData& curData) const override;
	};
}
