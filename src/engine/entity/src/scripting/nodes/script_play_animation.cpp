#include "script_play_animation.h"

#include "halley/core/graphics/sprite/animation_player.h"
#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "components/sprite_animation_component.h"

using namespace Halley;

IScriptNodeType::Result ScriptPlayAnimation::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto entity = environment.getEntity(node.getTargets().at(0));
	auto* spriteAnimation = entity.tryGetComponent<SpriteAnimationComponent>();
	if (spriteAnimation) {
		spriteAnimation->player.setSequence(node.getSettings()["sequence"].asString(""));
	}
	return { 0, ScriptNodeExecutionState::Done };
}
