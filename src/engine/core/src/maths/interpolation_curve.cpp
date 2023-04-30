#include "halley/maths/interpolation_curve.h"

using namespace Halley;

float InterpolationCurve::evaluate(float val) const
{
	return evaluateRaw(val) * scale;
}

float InterpolationCurve::evaluateRaw(float val) const
{
	// Before first point
	if (val < points.front().x) {
		return points.front().y;
	}

	// Between two points
	for (size_t i = 1; i < points.size(); ++i) {
		const float prevX = points[i - 1].x;
		const float nextX = points[i].x;

		if (val >= prevX && val < nextX) {
			const float t = (val - prevX) / (nextX - prevX);
			assert(t >= 0.0f);
			assert(t <= 1.0f);
			return lerp(points[i - 1].y, points[i].y, Tween<float>::applyCurve(t, tweens[i]));
		}
	}

	// After last point
	return points.back().y;
}

PrecomputedInterpolationCurve::PrecomputedInterpolationCurve(const InterpolationCurve& src)
	: scale(src.scale)
{
	constexpr float mult = 1.0f / static_cast<float>(nElements);
	for (size_t i = 0; i < nElements; ++i) {
		const float t = static_cast<float>(i) * mult;
		elements[i] = static_cast<uint8_t>(clamp(src.evaluateRaw(t), 0.0f, 1.0f) * 255.5f);
	}
}

float PrecomputedInterpolationCurve::evaluate(float t) const
{
	const float val = t * static_cast<float>(nElements);
	const auto v0 = std::floor(val);
	const auto idx0 = clamp(static_cast<size_t>(v0), static_cast<size_t>(0), nElements);
	const auto idx1 = clamp(static_cast<size_t>(v0 + 1), static_cast<size_t>(0), nElements);

	constexpr bool interpolate = true;
	if (interpolate) {
		return lerp(static_cast<float>(elements[idx0]) / 255.0f, static_cast<float>(elements[idx1]) / 255.0f, val - v0);
	} else {
		return static_cast<float>(elements[idx0]) / 255.0f;
	}
}
