#include "systems/movement_system.h"

void MovementSystem::update(Halley::Time time, MainFamily& e)
{
	auto& pos = e.position->position;
	auto& vel = e.velocity->velocity;
	
	pos += vel * time;

	if (pos.x < 0) {
		vel.x = abs(vel.x);
	} else if (pos.x > 1280) {
		vel.x = -abs(vel.x);
	}

	if (pos.y < 0) {
		vel.y = abs(vel.y);
	}
	else if (pos.y > 720) {
		vel.y = -abs(vel.y);
	}

	if (Halley::Random::getGlobal().getFloat(0.0f, 1.0f) < 0.001f) {
		getWorld().destroyEntity(e.entityId);
	}
}

REGISTER_SYSTEM(MovementSystem)
