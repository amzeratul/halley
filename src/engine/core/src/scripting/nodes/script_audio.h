#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptAudioEventData : public ScriptStateData<ScriptAudioEventData> {
	public:
		ScriptAudioEventData() = default;
		ScriptAudioEventData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;

		bool active = false;
	};

	class ScriptAudioEvent final : public ScriptNodeTypeBase<ScriptAudioEventData> {
	public:
		String getId() const override { return "audioEvent"; }
		String getName() const override { return "Audio Event"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/play_sound.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		bool hasDestructor(const ScriptGraphNode& node) const override;

		void doInitData(ScriptAudioEventData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptAudioEventData& data) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptAudioEventData& data) const override;
	};
}
