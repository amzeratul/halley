#include "systems/movement_system.h"

void MovementSystem::update(Halley::Time time, MainFamily& e)
{
	auto& pos = e.position->position;
	auto& vel = e.velocity->velocity;
	
	pos += vel * time;

	float margin = 1000;
	Halley::Rect4f bounds(-margin, -margin, 1280 + 2 * margin, 720 + 2 * margin);

	if (pos.x < bounds.getLeft()) {
		vel.x = abs(vel.x);
	} else if (pos.x > bounds.getRight()) {
		vel.x = -abs(vel.x);
	}

	if (pos.y < bounds.getTop()) {
		vel.y = abs(vel.y);
	}
	else if (pos.y > bounds.getBottom()) {
		vel.y = -abs(vel.y);
	}
}

void MovementSystem::onMessageReceived(const ExpireMessage& msg, MainFamily& e)
{
	getWorld().destroyEntity(e.entityId);
}

REGISTER_SYSTEM(MovementSystem)
