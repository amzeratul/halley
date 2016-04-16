#include "animation.h"
#include "animation_player.h"
#include "sprite.h"

using namespace Halley;

void AnimationPlayer::setAnimation(std::shared_ptr<Animation> v)
{
	animation = v;
}

void AnimationPlayer::update(Time time)
{
	spriteSheet = &animation->spriteSheet->getSprite("idle_side/01.png");
}

void AnimationPlayer::updateSprite(Sprite& sprite) const
{
	sprite.setMaterial(animation->material);
	sprite.setSprite(*spriteSheet);
}
