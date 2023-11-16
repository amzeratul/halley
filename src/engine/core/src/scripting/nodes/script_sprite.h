#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptSpriteAnimation final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "spriteAnimation"; }
		String getName() const override { return "Sprite Animation"; }
		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
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
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/set_facing.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptSpriteAlpha final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "spriteAlpha"; }
		String getName() const override { return "Sprite Alpha"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/sprite_alpha.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptSpriteActionPoint final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "actionPoint"; }
		String getName() const override { return "Action Point"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/action_point.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptColourGradient final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "colourGradient"; }
		String getName() const override { return "Colour Gradient"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/gradient.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
}
