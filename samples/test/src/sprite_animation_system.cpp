#include "../gen/cpp/systems/sprite_animation_system.h"

void SpriteAnimationSystem::update(Halley::Time, MainFamily& e)
{
	const char* ellaFrames[] = { "run_side/01.png", "run_side/02.png", "run_side/03.png", "run_side/04.png" };
	size_t nFrames = sizeof(ellaFrames) / sizeof(const char*);
	size_t curFrame = size_t(e.time->elapsed * 8) % nFrames;
	
	auto& sprite = e.sprite->sprite;
	sprite.setPos(e.position->position);
	sprite.setFlip(int(e.time->elapsed) % 2 == 0);
	sprite.setSprite(*getAPI().core->getResources().get<Halley::SpriteSheet>("sprites/ella.json"), ellaFrames[curFrame]);
}
