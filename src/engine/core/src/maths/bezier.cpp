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
	void bezierToLineSegments(Vector<Vector2f>& result, const T& curve, float tolerance, bool includeFirst = true)
	{
		if (includeFirst) {
			result.push_back(curve.getStartPoint());
		}
		bezierToLineSegmentsSubdivision(result, curve, tolerance);
	}

	std::pair<Vector2f, Vector2f> getSplineControlPoints(Vector2f prev, Vector2f a, Vector2f b, Vector2f next)
	{
		const auto abLen = (b - a).length();
		const auto n0 = ((a - prev).normalized() + (b - a).normalized()).normalized();
		const auto n1 = ((b - a).normalized() + (next - b).normalized()).normalized();
		return { a + n0 * (abLen / 3), b - n1 * (abLen / 3) };
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
	Vector<Vector2f> result;
	bezierToLineSegments(result, *this, 0.25f);
	return result;
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
	Vector<Vector2f> result;
	bezierToLineSegments(result, *this, 0.25f);
	return result;
}

void BezierCubic::toLineSegments(Vector<Vector2f>& output, bool includeFirst) const
{
	bezierToLineSegments(output, *this, 0.25f, includeFirst);
}

BezierCubicSpline BezierCubicSpline::fromEndPoints(Vector<Vector2f> points)
{
	if (points.size() < 2) {
		return points;
	}

	BezierCubicSpline result;
	result.points.reserve((points.size() - 1) * 3 + 1);
	result.points += points.front();

	for (size_t i = 1; i < points.size(); ++i) {
		const Vector2f a = points[i - 1];
		const Vector2f d = points[i];
		const Vector2f prev = i >= 2 ? points[i - 2] : a - (d - a);
		const Vector2f next = i < points.size() - 1 ? points[i + 1] : d + (d - a);

		const auto [b, c] = getSplineControlPoints(prev, a, d, next);

		result.points += b;
		result.points += c;
		result.points += d;
	}

	return result;
}

Vector<Vector2f> BezierCubicSpline::toLineSegments() const
{
	if (points.empty()) {
		return {};
	}
	if (points.size() < 4) {
		return Vector<Vector2f>{{ points.front(), }};
	}
	assert(points.size() % 3 == 1);

	Vector<Vector2f> result;
	result.push_back(points.front());

	const size_t nSegments = (points.size() - 1) / 3;
	for (size_t i = 0; i < nSegments; ++i) {
		size_t i0 = i * 3;
		BezierCubic(points[i0], points[i0 + 1], points[i0 + 2], points[i0 + 3]).toLineSegments(result, false);
	}

	return result;
}

