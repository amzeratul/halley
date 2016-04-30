#include "systems/time_system.h"

void TimeSystem::update(Halley::Time time, MainFamily& e)
{
	e.time->elapsed += float(time);

	if (e.time->elapsed > 2) {
		//sendMessage(e.entityId, ExpireMessage(e.time->elapsed));
	}
}

REGISTER_SYSTEM(TimeSystem)
