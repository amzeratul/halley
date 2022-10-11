#include "halley/maths/sphere.h"

using namespace Halley;

bool Sphere::contains(Vector3f point) const
{
	return (point - centre).squaredLength() <= radius * radius;
}

bool Sphere::overlaps(const Sphere& other) const
{
	const auto r = radius + other.radius;
	return (centre - other.centre).squaredLength() <= r * r;
}

Sphere Sphere::expand(float r) const
{
	return Sphere(centre, radius + r);
}

Sphere Sphere::getSpanningSphere(gsl::span<const Vector3f> points)
{
	// https://en.wikipedia.org/wiki/Bounding_sphere#Ritter's_bounding_sphere

	if (points.empty()) {
		return Sphere();
	}

	const auto x = points[0];
	Vector3f y;
	Vector3f z;

	float maxDist2 = 0;
	for (const auto& p: points) {
		auto dist2 = (p - x).squaredLength();
		if (dist2 > maxDist2) {
			maxDist2 = dist2;
			y = p;
		}
	}

	maxDist2 = 0;
	for (const auto& p: points) {
		auto dist2 = (p - y).squaredLength();
		if (dist2 > maxDist2) {
			maxDist2 = dist2;
			z = p;
		}
	}

	auto result = Sphere(0.5f * (y + z), 0.5f * (y - z).length());
	for (const auto& p: points) {
		const float dist = (p - result.centre).length();
		if (dist > result.radius) {
			const auto n = (p - result.centre).normalized();
			const float delta = 0.5f * dist;
			result = Sphere(result.centre + n * delta, result.radius + delta);
		}
	}

	return result;
}
