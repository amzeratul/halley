#include "../gen/cpp/systems/sprite_animation_system.h"

void SpriteAnimationSystem::update(Halley::Time, MainFamily& e)
{
	const char* ellaFrames[] = { "die/01.png", "die/02.png", "die/03.png", "die/04.png", "die/05.png" };
	size_t nFrames = sizeof(ellaFrames) / sizeof(const char*);
	size_t curFrame = size_t(e.time->elapsed * 5) % nFrames;
	
	auto& sprite = e.sprite->sprite;
	sprite.setPos(e.position->position);
	sprite.setFlip(int(e.time->elapsed) % 2 == 0);
	sprite.setSprite(*getAPI().core->getResources().get<Halley::SpriteSheet>("sprites/ella.json"), ellaFrames[curFrame]);
}
