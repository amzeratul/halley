#pragma once

#include "halley/data_structures/vector.h"
#include "halley/time/halleytime.h"
#include "halley/maths/vector2.h"

namespace Halley {

	class UIInertialDrag {
	public:
		std::optional<Vector2f> update(Time dt, float minStartSpeed, float minSpeed);

		void appendDelta(Vector2f deltaPos);
		void appendAbsolute(Vector2f pos);
		void stop();

    private:
		struct Entry {
			Vector2f deltaPos;
			Time timeSinceAdded = 0;
		};
        Vector<Entry> deltas;
		std::optional<Vector2f> inertialVel;
		std::optional<Vector2f> lastPos;
		bool hasUpdateThisFrame = false;
	};

}
