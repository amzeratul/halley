#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptSpriteAnimation final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "spriteAnimation"; }
		String getName() const override { return "Sprite Animation"; }
		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/play_animation.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptSpriteDirection final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "spriteDirection"; }
		String getName() const override { return "Sprite Direction"; }
		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/set_facing.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
