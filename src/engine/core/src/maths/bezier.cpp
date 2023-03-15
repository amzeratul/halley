#include "halley/maths/bezier.h"
#include "halley/maths/line.h"
using namespace Halley;

namespace {
	template <typename T>
	Vector<Vector2f> bezierToLineSegmentsFixed(const T& curve, size_t n)
	{
		const float scale = 1.0f / static_cast<float>(n - 1);
		
		Vector<Vector2f> result;
		result.reserve(n);
		for (size_t i = 0; i < n; ++i) {
			const float t = static_cast<float>(i) * scale;
			result.push_back(curve.evaluate(t));
		}

		return result;
	}

	template <typename T>
	void bezierToLineSegmentsSubdivision(Vector<Vector2f>& result, const T& curve, float tolerance)
	{
		if (curve.isFlat(tolerance)) {
			result.push_back(curve.getEndPoint());
		} else {
			const auto [c0, c1] = curve.splitAt(0.5f);
			bezierToLineSegmentsSubdivision(result, c0, tolerance);
			bezierToLineSegmentsSubdivision(result, c1, tolerance);
		}
	}

	template <typename T>
	Vector<Vector2f> bezierToLineSegmentsSubdivision(const T& curve, float tolerance)
	{
		Vector<Vector2f> result;
		result.push_back(curve.getStartPoint());
		bezierToLineSegmentsSubdivision(result, curve, tolerance);
		return result;
	}
}

std::pair<BezierQuadratic, BezierQuadratic> BezierQuadratic::splitAt(float t) const
{
	// De Casteljau's Algorithm
	const Vector2f l1 = lerp(p0, p1, t);
	const Vector2f r1 = lerp(p1, p2, t);
	const Vector2f l2 = lerp(l1, r1, t);

	return { BezierQuadratic(p0, l1, l2), BezierQuadratic(l2, r1, p2) };
}

bool BezierQuadratic::isFlat(float tolerance) const
{
	return Line(p0, (p2 - p0).normalized()).getDistance(p1) < tolerance;
}

Vector<Vector2f> BezierQuadratic::toLineSegments() const
{
	return bezierToLineSegmentsSubdivision(*this, 0.25f);
}

std::pair<BezierCubic, BezierCubic> BezierCubic::splitAt(float t) const
{
	// De Casteljau's Algorithm
	const Vector2f m = lerp(p1, p2, t);
	const Vector2f l1 = lerp(p0, p1, t);
	const Vector2f r2 = lerp(p2, p3, t);
	const Vector2f l2 = lerp(l1, m, t);
	const Vector2f r1 = lerp(m, r2, t);
	const Vector2f l3 = lerp(l2, r1, t);

	return { BezierCubic(p0, l1, l2, l3), BezierCubic(l3, r1, r2, p3) };
}

bool BezierCubic::isFlat(float tolerance) const
{
	const float t = 16.0f * tolerance * tolerance;

	const float ux = 3.0f * p1.x - 2.0f * p0.x - p3.x;
	const float uy = 3.0f * p1.y - 2.0f * p0.y - p3.y;
	const float vx = 3.0f * p2.x - 2.0f * p3.x - p0.x;
	const float vy = 3.0f * p2.y - 2.0f * p3.y - p0.y;

	return std::max(ux * ux, vx * vx) + std::max(uy * uy, vy * vy) <= t;

	/*
	const auto baseLine = Line(p0, (p3 - p0).normalized());
	return baseLine.getDistance(p1) < tolerance && baseLine.getDistance(p2) < tolerance;
	*/
}

Vector<Vector2f> BezierCubic::toLineSegments() const
{
	return bezierToLineSegmentsSubdivision(*this, 0.25f);
}
