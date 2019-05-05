#include "halley/maths/quaternion.h"
#include "halley/maths/matrix4.h"
#include <cmath>
using namespace Halley;

Quaternion::Quaternion(Vector3f axis, Angle1f angle)
{
	float sine, cosine;
	(angle * 0.5f).sincos(sine, cosine);
	const auto u = axis.unit() * sine;
	w = cosine;
	x = u.x;
	y = u.y;
	z = u.z;
}

Quaternion::Quaternion(Vector3f v)
	: w(0)
	, x(v.x)
	, y(v.y)
	, z(v.z)
{
}

Quaternion Quaternion::operator*(const Quaternion& o) const
{
	return Quaternion(
		w * o.w - x * o.x - y * o.y - z * o.z,
		w * o.x + x * o.w + y * o.z - z * o.y,
		w * o.y - x * o.z + y * o.w + z * o.x,
		w * o.z + x * o.y - y * o.x + z * o.w
	);
}

Vector3f Quaternion::operator*(Vector3f other) const
{
	const auto p = Quaternion(other);
	const auto& q = *this;
	return (q * p * q.conjugated()).toVector3f();
}

bool Quaternion::operator==(const Quaternion& other) const
{
	return w == other.w && x == other.x && y == other.y && z == other.z;
}

bool Quaternion::operator!=(const Quaternion& other) const
{
	return w != other.w || x != other.x || y != other.y || z != other.z;
}

void Quaternion::loadIdentity()
{
	w = 1;
	x = 0;
	y = 0;
	z = 0;
}

void Quaternion::normalise()
{
	*this = normalised();
}

Quaternion Quaternion::normalised() const
{
	const float iLen = 1.0f / std::sqrt(w * w + x * x + y * y + z * z);
	return Quaternion(w * iLen, x * iLen, y * iLen, z * iLen);
}

Quaternion Quaternion::inverse() const
{
	const float iLen = 1.0f / std::sqrt(w * w + x * x + y * y + z * z);
	return Quaternion(w * iLen, -x * iLen, -y * iLen, -z * iLen);
}

Quaternion Quaternion::conjugated() const
{
	return Quaternion(w, -x, -y, -z);
}

Vector3f Quaternion::toVector3f() const
{
	return Vector3f(x, y, z);
}

Quaternion Quaternion::lookAt(const Vector3f& dir, const Vector3f& worldUp)
{
	// See https://stackoverflow.com/questions/52413464/look-at-quaternion-using-up-vector/52551983#52551983
	const Vector3f f = dir.normalized(); // Front
	const Vector3f r = worldUp.cross(f).normalized(); // Right
	const Vector3f u = f.cross(r); // Up

	const float trace = r.x + u.y + f.z;
	if (trace > 0.0f) {
		const float s = 0.5f / std::sqrt(trace + 1.0f);
		return Quaternion(
			0.25f / s,
			(u.z - f.y) * s,
			(f.x - r.z) * s,
			(r.y - u.x) * s
		);
	} else {
		if (r.x > u.y && r.x > f.z) {
			const float s = 2.0f * std::sqrt(1.0f + r.x - u.y - f.z);
			return Quaternion(
				(u.z - f.y) / s,
				0.25f * s,
				(u.x + r.y) / s,
				(f.x + r.z) / s
			);
		} else if (u.y > f.z) {
			const float s = 2.0f * std::sqrt(1.0f + u.y - r.x - f.z);
			return Quaternion(
				(f.x - r.z) / s,
				(u.x + r.y) / s,
				0.25f * s,
				(f.y + u.z) / s
			);
		} else {
			const float s = 2.0f * std::sqrt(1.0f + f.z - r.x - u.y);
			return Quaternion(
				(r.y - u.x) / s,
				(f.x + r.z) / s,
				(f.y + u.z) / s,
				0.25f * s
			);
		}
	}
}
