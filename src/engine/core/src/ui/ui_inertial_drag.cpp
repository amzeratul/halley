#include "halley/ui/ui_inertial_drag.h"

#include "halley/utils/algorithm.h"

using namespace Halley;

std::optional<Vector2f> UIInertialDrag::update(Time dt, float minStartSpeed, float minSpeed)
{
	std::optional<Vector2f> result;

	for (auto& d: deltas) {
		d.timeSinceAdded += dt;
	}

	constexpr Time timeLimit = 0.1;
	std_ex::erase_if(deltas, [&](const Entry& d) { return d.timeSinceAdded > timeLimit; });

	if (hasUpdateThisFrame) {
		//Logger::logInfo("Read back: " + toString(deltas.back().deltaPos));
		result = deltas.back().deltaPos;
	} else {
		auto& vel = inertialVel;
		if (!vel && deltas.size() >= 3 && deltas.front().timeSinceAdded > 0.05) {
			Vector2f ds;
			for (const auto& d: deltas) {
				ds += d.deltaPos;
			}
			const float dt = static_cast<float>(deltas.front().timeSinceAdded);
			vel = ds / dt;
			//Logger::logInfo("Inertial drag started with " + toString(*vel) + " (" + ds + "px / " + dt + "s)");
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

void UIInertialDrag::appendDelta(Vector2f deltaPos)
{
	//Logger::logInfo("Append: " + toString(deltaPos));
	deltas.emplace_back(Entry{ deltaPos, 0.0 });
	inertialVel.reset();
	hasUpdateThisFrame = true;
}

void UIInertialDrag::appendAbsolute(Vector2f pos)
{
	if (lastPos) {
		appendDelta(pos - *lastPos);
	}
	lastPos = pos;
}

void UIInertialDrag::stop()
{
	deltas.clear();
	inertialVel = Vector2f();
	lastPos = std::nullopt;
}
