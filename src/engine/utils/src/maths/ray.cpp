#include "halley/maths/ray.h"
using namespace Halley;

Ray::Ray()
{
}

Ray::Ray(Vector2f start, Vector2f dir)
	: p(start)
	, dir(dir)
{
}

Maybe<float> Ray::castCircle(Vector2f centre, float radius) const
{
	// Is ahead of ray?
	const Vector2f localCentre = centre - p;
	if (dir.dot(localCentre) < 0) {
		// Behind ray
		return {};
	}

	// Distance between center of circle and ray
	const Vector2f n = dir.orthoLeft();
	const float distToCentre = n.dot(localCentre);
	if (distToCentre > radius) {
		// Ray doesn't overlap circle
		return {};
	}

	// Closest point (in local space) on ray to centre
	const Vector2f localClosest = localCentre - n * distToCentre;

	// Intersect point, closest point, and center of circle form of a right triangle, use it to find intersect point
	const float distToClosest = dir.dot(localClosest);
	const float intersectionDepth = sqrt(radius * radius - distToCentre * distToCentre);
	
	return distToClosest - intersectionDepth;
}

Maybe<float> Ray::castLineSegment(Vector2f a, Vector2f b) const
{
	// From http://geomalgorithms.com/a05-_intersect-1.html

	const Vector2f u = dir;
	const Vector2f ut = u.orthoLeft();
	const Vector2f v = b - a;
	const Vector2f vt = v.orthoLeft();
	const float denom = vt.dot(dir);
	if (denom == 0) {
		// Parallel
		return {};
	}

	const Vector2f w = p - a;
	const float s = -vt.dot(w) / denom;
	if (s < 0) {
		// Behind ray
		return {};
	}

	const float t = -ut.dot(w) / denom;
	if (t < 0 || t > 1) {
		// Outside segment
		return {};
	}

	return s;
}
