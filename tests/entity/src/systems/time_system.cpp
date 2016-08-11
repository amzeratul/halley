#include "systems/time_system.h"

class TimeSystem final : public TimeSystemBase<TimeSystem> {
public:
	void update(Halley::Time time, MainFamily& e)
	{
		e.time.elapsed += float(time);

		if (e.time.elapsed > 5) {
			sendMessage(e.entityId, ExpireMessage(e.time.elapsed));
		}
	}
};

REGISTER_SYSTEM(TimeSystem)
