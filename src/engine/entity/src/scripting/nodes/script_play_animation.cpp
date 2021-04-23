#include "script_play_animation.h"

#include "halley/core/graphics/sprite/animation_player.h"
#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "components/sprite_animation_component.h"

using namespace Halley;

std::pair<String, std::vector<ColourOverride>> ScriptPlayAnimation::getDescription(const ScriptGraphNode& node) const
{
	String text;
	std::vector<ColourOverride> cols;

	text += "Play sequence \"";
	cols.emplace_back(text.length(), Colour4f(0.97f, 0.35f, 0.35f));
	text += toString(node.getSettings()["sequence"].asString(""));
	cols.emplace_back(text.length(), std::optional<Colour4f>());
	text += "\" on target.";

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
