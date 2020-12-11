#include "halley/maths/base_transform.h"
using namespace Halley;

Base2D::Base2D()
{}

Base2D::Base2D(Vector2f u, Vector2f v)
	: u(u), v(v)
{
	// Compute inverse
	const float det = 1.0f / u.cross(v);
	invU = det * Vector2f(v.y, -u.y);
	invV = det * Vector2f(-v.x, u.x);
}

Base2D::Base2D(const ConfigNode& node)
	: Base2D(node["u"].asVector2f(), node["v"].asVector2f())
{
}

Base2D::Base2D(Vector2f u, Vector2f v, Vector2f invU, Vector2f invV)
	: u(u), v(v), invU(invU), invV(invV)
{
}

Vector2f Base2D::transform(Vector2f point) const
{
	return transform(point, u, v);
}

Vector2f Base2D::inverseTransform(Vector2f point) const
{
	return transform(point, invU, invV);
}

Polygon Base2D::transform(const Polygon& poly) const
{
	auto vs = poly.getVertices();
	for (auto& v: vs) {
		v = transform(v);
	}
	return Polygon(std::move(vs));
}

Polygon Base2D::inverseTransform(const Polygon& poly) const
{
	auto vs = poly.getVertices();
	for (auto& v: vs) {
		v = inverseTransform(v);
	}
	return Polygon(std::move(vs));
}

Vector2f Base2D::transform(Vector2f point, Vector2f u, Vector2f v)
{
	return point.x * u + point.y * v;
}

Base2D Base2D::getInverse() const
{
	return Base2D(invU, invV, u, v);
}

ConfigNode Base2D::toConfigNode() const
{
	ConfigNode::MapType result;
	result["u"] = u;
	result["v"] = v;
	return ConfigNode(result);
}

