#include "halley/maths/base_transform.h"
#include "halley/bytes/byte_serializer.h"
using namespace Halley;

Base2D::Base2D(const ConfigNode& node)
	: Base2D(node["u"].asVector2f(), node["v"].asVector2f())
{
}

Base2D::Base2D(Vector2f u, Vector2f v, Vector2f invU, Vector2f invV)
	: u(u), v(v), invU(invU), invV(invV)
{
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

void Base2D::serialize(Serializer& s) const
{
	s << u;
	s << v;
}

void Base2D::deserialize(Deserializer& s)
{
	Vector2f a, b;
	s >> a;
	s >> b;
	*this = Base2D(a, b);
}

