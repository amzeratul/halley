#include "halley/ui/ui_inertial_drag.h"

using namespace Halley;

std::optional<Vector2f> UIInertialDrag::update(Time dt, float minStartSpeed, float minSpeed)
{
	std::optional<Vector2f> result;

	if (dt > 0.000001) {
		for (auto& d: deltas) {
			if (d.needsTime) {
				d.deltaTime = dt;
				d.needsTime = false;
			}
		}
	}

	if (hasUpdateThisFrame) {
		if (dt < 0.000001) {
			return {};
		}
		result = deltas.back().deltaPos * static_cast<float>(dt / deltas.back().deltaTime);
	} else {
		auto& vel = inertialVel;
		if (!vel && deltas.size() >= 3) {
			Vector2f ds;
			Time dt = 0;
			for (const auto& d: deltas) {
				ds += d.deltaPos;
				dt += d.deltaTime;
			}
			vel = ds / static_cast<float>(dt);
			deltas.clear();
			if (vel->length() < minStartSpeed) {
				vel.reset();
			}
		}

		if (vel && vel->length() >= minSpeed) {
			result = *vel * static_cast<float>(dt);
			vel = damp(*vel, Vector2f(), 5.0f, static_cast<float>(dt));
		}
	}
	hasUpdateThisFrame = false;

	return result;
}

void UIInertialDrag::appendDelta(Vector2f deltaPos, std::optional<Time> deltaTime)
{
	deltas.emplace_back(Entry{ deltaPos, deltaTime.value_or(0), !deltaTime });
	if (deltas.size() > 3) {
		deltas.erase(deltas.begin());
	}
	inertialVel.reset();
	hasUpdateThisFrame = true;
}

void UIInertialDrag::appendAbsolute(Vector2f pos, std::optional<Time> deltaTime)
{
	if (lastPos) {
		appendDelta(pos - *lastPos, deltaTime);
	}
	lastPos = pos;
}

void UIInertialDrag::stop()
{
	deltas.clear();
	inertialVel = Vector2f();
	lastPos = std::nullopt;
}
