#include "script_play_animation.h"

#include "halley/core/graphics/sprite/animation_player.h"
#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "world.h"
#include "components/sprite_animation_component.h"

using namespace Halley;

std::vector<IScriptNodeType::SettingType> ScriptPlayAnimation::getSettingTypes() const
{
	return { SettingType{ "sequence", "Halley::String", std::vector<String>{"default"} } };
}

std::pair<String, std::vector<ColourOverride>> ScriptPlayAnimation::getDescription(const ScriptGraphNode& node, const World& world) const
{
	String text;
	std::vector<ColourOverride> cols;

	const EntityId targetId = node.getTargets().empty() ? EntityId() : node.getTargets()[0];
	const ConstEntityRef target = world.getEntity(targetId);
	const String targetName = target.isValid() ? target.getName() : "<unknown>";

	text += "Play sequence \"";
	cols.emplace_back(text.length(), Colour4f(0.97f, 0.35f, 0.35f));
	text += toString(node.getSettings()["sequence"].asString(""));
	cols.emplace_back(text.length(), std::optional<Colour4f>());
	text += "\" on entity \"";
	cols.emplace_back(text.length(), Colour4f(0.97f, 0.35f, 0.35f));
	text += targetName;
	cols.emplace_back(text.length(), std::optional<Colour4f>());
	text += "\".";

	return { std::move(text), std::move(cols) };
}

IScriptNodeType::Result ScriptPlayAnimation::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto entity = environment.getEntity(node.getTargets().at(0));
	auto* spriteAnimation = entity.tryGetComponent<SpriteAnimationComponent>();
	if (spriteAnimation) {
		spriteAnimation->player.setSequence(node.getSettings()["sequence"].asString(""));
	}
	return { 0, ScriptNodeExecutionState::Done };
}
