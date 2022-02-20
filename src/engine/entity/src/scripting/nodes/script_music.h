#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptPlayMusic final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "playMusic"; }
		String getName() const override { return "Music Play"; }
		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/play_music.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptStopMusic final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "stopMusic"; }
		String getName() const override { return "Music Stop"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/stop_music.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
