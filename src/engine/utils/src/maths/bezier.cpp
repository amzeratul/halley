#include "halley/maths/bezier.h"
using namespace Halley;

template <typename T>
static std::vector<Vector2f> bezierToLineSegments(const T& curve)
{
	size_t n = 30;
	const float scale = 1.0f / static_cast<float>(n - 1);
	
	std::vector<Vector2f> result;
	result.reserve(n);
	for (size_t i = 0; i < n; ++i) {
		const float t = static_cast<float>(i) * scale;
		result.push_back(curve.evaluate(t));
	}

	return result;
}

std::vector<Vector2f> BezierQuadratic::toLineSegments() const
{
	return bezierToLineSegments(*this);
}

std::vector<Vector2f> BezierCubic::toLineSegments() const
{
	return bezierToLineSegments(*this);
}
