#include "halley/maths/triangle.h"
#include "halley/maths/polygon.h"

using namespace Halley;

Vector3f Triangle::getBarycentricCoordinates(Vector2f p) const
{
	const float div = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
	const float w1 = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / div;
	const float w2 = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / div;
	const float w3 = 1 - w1 - w2;
	return Vector3f(w1, w2, w3);
}

Rect4f Triangle::getBounds() const
{
	const float x0 = std::min(a.x, std::min(b.x, c.x));
	const float y0 = std::min(a.y, std::min(b.y, c.y));
	const float x1 = std::max(a.x, std::max(b.x, c.x));
	const float y1 = std::max(a.y, std::max(b.y, c.y));
	return Rect4f(x0, y0, x1 - x0, y1 - y0);
}

bool Triangle::contains(Vector2f p) const
{
	const auto c = getBarycentricCoordinates(p);
	return c.x >= 0 && c.y >= 0 && c.z >= 0;
}

float Triangle::getArea() const
{
	return std::abs((b - a).cross(c - a)) / 2;
}

float Triangle::getPerimeter() const
{
	return (b - a).length() + (c - b).length() + (a - c).length();
}

Polygon Triangle::toPolygon() const
{
	return Polygon(VertexList{ a, b, c });
}
