#include "halley/maths/circle.h"

#include "halley/maths/line.h"
#include "halley/maths/random.h"
#include "halley/maths/triangle.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

Circle::Circle(const LineSegment& segment)
{
	centre = 0.5f * (segment.a + segment.b);
	radius = 0.5f * (segment.a - segment.b).length();
}

bool Circle::contains(Vector2f point) const
{
	return (point - centre).squaredLength() <= radius * radius;
}

bool Circle::overlaps(const Circle& circle) const
{
	const float r = radius + circle.radius;
	return (circle.centre - centre).squaredLength() <= r * r;
}

Circle Circle::expand(float r) const
{
	return Circle(centre, radius + r);
}

Rect4f Circle::getAABB() const
{
	return Rect4f(centre - Vector2f(radius, radius), centre + Vector2f(radius, radius));
}

Vector2f Circle::project(Vector2f point) const
{
	return centre + (point - centre).normalized() * radius;
}

float Circle::getDistanceTo(Vector2f point) const
{
	return std::max((centre - point).length() - radius, 0.0f);
}

float Circle::getDistanceTo(const Circle& circle) const
{
	return std::max((centre - circle.centre).length() - radius - circle.radius, 0.0f);
}

Circle Circle::getSpanningCircle(const Vector<Vector2f>& points)
{

	if (points.empty()) {
		return Circle(Vector2f(), 0);
	}
	
	Vector2f centre;
	for (auto& p: points) {
		centre += p;
	}
	centre /= float(points.size());

	float radius2 = 0;
	for (auto& p: points) {
		const auto d = (p - centre).squaredLength();
		if (d > radius2) {
			radius2 = d;
		}
	}

	return Circle(centre, std::sqrt(radius2));
}

Circle Circle::getSpanningCircle2(Vector<Vector2f> points)
{
	// Matoušek, Sharir, Welzl's algorithm
	// https://en.wikipedia.org/wiki/Smallest-circle_problem#Welzl's_algorithm
	
	shuffle(points.begin(), points.end(), Random::getGlobal());

	const auto span = gsl::span<Vector2f>(points);
	if (span.size() <= 3) {
		return getSpanningCircleTrivial(span);
	}

	return msw(span.subspan(3, span.size() - 3), gsl::span<Vector2f, 3>(points.data(), 3));
}

Circle Circle::msw(gsl::span<Vector2f> ps, gsl::span<Vector2f, 3> rs)
{
	if (ps.empty()) {
		return getSpanningCircleTrivial(rs);
	}
	auto& p = ps[0];
	const auto d = msw(ps.subspan(1), rs);
	if (d.contains(p)) {
		return d;
	}
	auto& q = mswNonBase(p, rs);
	std::swap(p, q);
	return msw(ps, rs);
}

Vector2f& Circle::mswNonBase(Vector2f p, gsl::span<Vector2f, 3> rs)
{
	for (int i = 0; i < 3; ++i) {
		std::array<Vector2f, 3> ps;
		ps[0] = p;
		int pos = 1;
		for (int j = 0; j < 3; ++j) {
			if (j != i) {
				ps[pos++] = rs[j];
			}
		}

		if (getSpanningCircleTrivial(gsl::span<Vector2f>(ps)).contains(rs[i])) {
			return rs[i];
		}
	}

	throw Exception("Unable to find nonbase point in msw algorithm", HalleyExceptions::Utils);
}

Circle Circle::getSpanningCircleTrivial(gsl::span<Vector2f> ps)
{
	if (ps.size() == 3) {
		// If obtuse, use longest edge
		auto a = (ps[0] - ps[1]).squaredLength();
		auto b = (ps[1] - ps[2]).squaredLength();
		auto c = (ps[2] - ps[0]).squaredLength();
		if (a > b + c) {
			return Circle(LineSegment(ps[0], ps[1]));
		} else if (b > a + c) {
			return Circle(LineSegment(ps[1], ps[2]));
		} else if (c > a + b) {
			return Circle(LineSegment(ps[2], ps[0]));
		} else {
			// Not obtuse, use the circumscribed circle
			return Triangle(ps[0], ps[1], ps[2]).getCircumscribedCircle();
		}
	} else if (ps.size() == 2) {
		return Circle(LineSegment(ps[0], ps[1]));
	} else if (ps.size() == 1) {
		return Circle(ps[0], 0);
	} else if (ps.size() == 0) {
		return Circle();
	} else {
		throw Exception("Invalid spanning circle trivial case in msw algorithm", HalleyExceptions::Utils);
	}
}

Circle Circle::getCircleTangentToAngle(Vector2f A, Vector2f B, Vector2f C, float radius)
{
	// Let P be the centre of the circle
	// alpha is the angle at B
	// d is the distance from B, along the ABC bisector
	// d = r / sin(alpha/2)
	// Using sin(alpha/2) = sqrt((1 - cos(alpha)) / 2)

	const float cosAlpha = (B - A).normalized().dot((C - B).normalized());
	if (cosAlpha > 0.9999f) {
		throw Exception("Attempting to find circle tangent to collinear points", HalleyExceptions::Utils);
	}
	const float halfSinAlpha = std::sqrt((1.0f - cosAlpha) / 2.0f);
	const float d = radius / halfSinAlpha;

	const auto bisectorN = (((A - B).normalized() + (C - B).normalized()) * 0.5f).normalized();
	const auto P = B + d * bisectorN;
	return Circle(P, radius);
}
