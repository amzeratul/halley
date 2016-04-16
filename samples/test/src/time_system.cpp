#include "../gen/cpp/systems/time_system.h"

void TimeSystem::update(Halley::Time time, MainFamily& e)
{
	e.time->elapsed += float(time);
}
