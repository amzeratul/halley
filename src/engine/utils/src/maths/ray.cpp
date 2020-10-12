#include "halley/maths/ray.h"
#include "halley/maths/polygon.h"
using namespace Halley;

Ray::Ray()
{
}

Ray::Ray(Vector2f start, Vector2f dir)
	: p(start)
	, dir(dir)
{
}

std::optional<std::pair<float, Vector2f>> Ray::castCircle(Vector2f centre, float radius) const
{
	const Vector2f localCentre = centre - p;
	// Is ray already inside?
	if (const float sqLen = (p - centre).squaredLength(); sqLen < radius * radius) {
		if (sqLen > 0.00001f) {
			return std::pair<float, Vector2f>(0.0f, (p - centre).normalized());
		}
		else {
			return std::pair<float, Vector2f>(0.0f, Vector2f(0.0f, 1.0f));
		}
	}
	
	// Is ahead of ray?
	if (dir.dot(localCentre) < 0) {
		// Behind ray
		return {};
	}

	// Distance between center of circle and ray
	const Vector2f n = dir.orthoLeft();
	const float distToCentre = std::abs(n.dot(localCentre));
	if (distToCentre > radius) {
		// Ray doesn't overlap circle
		return {};
	}

	// Closest point (in local space) on ray to centre
	const Vector2f localClosest = localCentre - n * distToCentre;

	// Intersect point, closest point, and center of circle form of a right triangle, use it to find intersect point
	const float distToClosest = dir.dot(localClosest);
	const float intersectionDepth = sqrt(radius * radius - distToCentre * distToCentre);
	const float dist = distToClosest - intersectionDepth;
	const Vector2f intersectionPoint = p + dir * dist;
	const Vector2f normal = (intersectionPoint - centre).normalized();

	//Logger::logInfo(" circ [" + toString(dist) + ", " + dir + "] - " + normal);

	Ensures(!std::isnan(dist));
	Ensures(!std::isnan(normal.x));
	Ensures(!std::isnan(normal.y));
	
	return std::pair<float, Vector2f>(dist, normal);
}

std::optional<std::pair<float, Vector2f>> Ray::castLineSegment(Vector2f a, Vector2f b) const
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

	const Vector2f normal = vt.normalized();

	Ensures(!std::isnan(s));
	Ensures(!std::isnan(normal.x));
	Ensures(!std::isnan(normal.y));

	//Logger::logInfo(" line [" + toString(s) + ", " + dir + "] - " + normal);
	
	return std::pair<float, Vector2f>(s, dir.dot(normal) > 0 ? -normal : normal);
}

std::optional<std::pair<float, Vector2f>> Ray::castPolygon(const Polygon& polygon) const
{
	const auto& boundingCircle = polygon.getBoundingCircle();
	if (!castCircle(boundingCircle.getCentre(), boundingCircle.getRadius())) {
		return {};
	}

	const auto& vertices = polygon.getVertices();
	const auto vertexCount = int(vertices.size());
	std::optional<std::pair<float, Vector2f>> closestIntersection;
	int numIntersections = 0;
	for (auto i = 0; i < vertexCount; i++) {
		if (auto intersection = castLineSegment(vertices[i], vertices[(i+1)%(vertexCount)])) {
			if (!closestIntersection || intersection->first < closestIntersection->first) {
				closestIntersection = intersection;
			}
			++numIntersections;
		}
	}

	if (numIntersections % 2 == 1) { // We're already inside
		return std::pair<float, Vector2f>(0.0f, Vector2f());
	}

	return closestIntersection;
}
