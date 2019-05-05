#include "halley/maths/quaternion.h"
#include <cmath>
using namespace Halley;

Quaternion::Quaternion(Vector3f axis, Angle1f angle)
{
	float sine, cosine;
	(angle * 0.5f).sincos(sine, cosine);
	const auto u = axis.unit() * sine;
	a = cosine;
	b = u.x;
	c = u.y;
	d = u.z;
}

Quaternion Quaternion::operator*(const Quaternion& o) const
{
	return Quaternion(
		a * o.a - b * o.b - c * o.c - d * o.d,
		a * o.b + b * o.a + c * o.d - d * o.c,
		a * o.c - b * o.d + c * o.a + d * o.b,
		a * o.d + b * o.c - c * o.b + d * o.a
	);
}

Vector3f Quaternion::operator*(Vector3f other) const
{
	// Is this right?
	const auto q = *this * Quaternion(1.0, other.x, other.y, other.z);
	const float w = 1.0f / q.a;
	return Vector3f(q.b * w, q.c * w, q.d * w);
}

void Quaternion::loadIdentity()
{
	a = 1;
	b = 0;
	c = 0;
	d = 0;
}

void Quaternion::normalise()
{
	*this = normalised();
}

Quaternion Quaternion::normalised() const
{
	Quaternion result;
	const float iLen = 1.0f / std::sqrt(a * a + b * b + c * c + d * d);
	result.a *= iLen;
	result.b *= iLen;
	result.c *= iLen;
	result.d *= iLen;
	return result;
}

Quaternion Quaternion::inverse() const
{
	const float iLen = 1.0f / std::sqrt(a * a + b * b + c * c + d * d);
	return Quaternion(a * iLen, -b * iLen, -c * iLen, -d * iLen);
}

Quaternion Quaternion::conjugated() const
{
	return Quaternion(a, -b, -c, -d);
}
