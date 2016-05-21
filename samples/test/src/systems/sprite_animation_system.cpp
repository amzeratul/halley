#include "systems/sprite_animation_system.h"

void SpriteAnimationSystem::update(Halley::Time time, MainFamily& e)
{
	auto& sprite = e.sprite->sprite;

	auto vel = e.velocity->velocity;
	int dir = std::abs(vel.y) > std::abs(vel.x) ? (vel.y < 0 ? 0 : 2) : (vel.x < 0 ? 3 : 1);

	auto& player = e.spriteAnimation->player;
	player.setDirection(dir);
	player.update(time);
	player.updateSprite(sprite);
}

REGISTER_SYSTEM(SpriteAnimationSystem)
