#include "../gen/cpp/systems/movement_system.h"

void MovementSystem::update(Halley::Time time, MainFamily& e)
{
	e.position->position += e.velocity->velocity * time;
}
