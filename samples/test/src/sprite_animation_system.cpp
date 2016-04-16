#include "../gen/cpp/systems/sprite_animation_system.h"

void SpriteAnimationSystem::update(Halley::Time time, MainFamily& e)
{
	auto& sprite = e.sprite->sprite;
	sprite.setPos(e.position->position);

	auto& player = e.spriteAnimation->player;
	player.update(time);
	player.updateSprite(sprite);
}
