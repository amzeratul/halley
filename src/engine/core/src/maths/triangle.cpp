#include "halley/maths/triangle.h"
#include "halley/maths/polygon.h"
#include "halley/maths/random.h"

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

float Triangle::getDistance(Vector2f p) const
{
	if (contains(p)) {
		return 0;
	}
	const auto s0 = LineSegment(a, b);
	const auto s1 = LineSegment(b, c);
	const auto s2 = LineSegment(c, a);
	return std::min(s0.getDistance(p), std::min(s1.getDistance(p), s2.getDistance(p)));
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

Circle Triangle::getCircumscribedCircle() const
{
	// Find two perpendicular bisectors
	const auto b0 = Line(0.5f * (a + b), (a - b).normalized().orthoRight());
	const auto b1 = Line(0.5f * (b + c), (b - c).normalized().orthoRight());

	// Circle centre is where they cross
	const auto centre = b0.intersection(b1);
	if (!centre) {
		return Circle();
	}

	// Radius is just the distance to any vertex
	const auto radius = (a - *centre).length();

	return Circle(*centre, radius);
}

Vector2f Triangle::getRandomPoint(Random& rng) const
{
	const auto ab = b - a;
	const auto ac = c - a;
	auto u = rng.getFloat(0.0f, 1.0f);
	auto v = rng.getFloat(0.0f, 1.0f);
	if (u + v > 1.0f) {
		u = 1 - u;
		v = 1 - v;
	}
	return (ab * u + ac * v) + a;
}

Triangle Triangle::operator+(Vector2f v) const
{
	return Triangle(a + v, b + v, c + v);
}

Triangle Triangle::operator-(Vector2f v) const
{
	return Triangle(a - v, b - v, c - v);
}

Triangle Triangle::operator*(Vector2f v) const
{
	return Triangle(a * v, b * v, c * v);
}

Triangle Triangle::operator/(Vector2f v) const
{
	return Triangle(a / v, b / v, c / v);
}

Triangle Triangle::operator*(float scalar) const
{
	return Triangle(a * scalar, b * scalar, c * scalar);
}

Triangle Triangle::operator/(float scalar) const
{
	return Triangle(a / scalar, b / scalar, c / scalar);
}

Triangle& Triangle::operator+=(Vector2f v)
{
	a += v;
	b += v;
	c += v;
	return *this;
}

Triangle& Triangle::operator-=(Vector2f v)
{
	a -= v;
	b -= v;
	c -= v;
	return *this;
}

Triangle& Triangle::operator*=(Vector2f v)
{
	a *= v;
	b *= v;
	c *= v;
	return *this;
}

Triangle& Triangle::operator/=(Vector2f v)
{
	a /= v;
	b /= v;
	c /= v;
	return *this;
}

Triangle& Triangle::operator*=(float scalar)
{
	a *= scalar;
	b *= scalar;
	c *= scalar;
	return *this;
}

Triangle& Triangle::operator/=(float scalar)
{
	a /= scalar;
	b /= scalar;
	c /= scalar;
	return *this;
}

bool Triangle::operator==(const Triangle& other) const
{
	return a == other.a && b == other.b && c == other.c;
}

bool Triangle::operator!=(const Triangle& other) const
{
	return a != other.a || b != other.b || c != other.c;
}
