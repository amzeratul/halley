#include "halley/maths/quaternion.h"
#include "halley/maths/matrix4.h"
#include <cmath>
using namespace Halley;

Quaternion::Quaternion(Vector3f axis, Angle1f angle)
{
	const auto a = angle * 0.5f;
	const float sine = a.sin();
	const float cosine = a.cos();
	
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
	const float iLen2 = 1.0f / (w * w + x * x + y * y + z * z);
	return Quaternion(w * iLen2, -x * iLen2, -y * iLen2, -z * iLen2);
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
	const Vector3f fw = dir.normalized(); // Front
	const Vector3f lf = fw.cross(worldUp).normalized(); // Left
	const Vector3f up = lf.cross(fw); // Up

	const auto a0 = lf;
	const auto a1 = up;
	const auto a2 = fw;
	const auto a00 = a0.x;
	const auto a10 = a0.y;
	const auto a20 = a0.z;
	const auto a01 = a1.x;
	const auto a11 = a1.y;
	const auto a21 = a1.z;
	const auto a02 = a2.x;
	const auto a12 = a2.y;
	const auto a22 = a2.z;

	const float trace = a00 + a11 + a22;
	if (trace > 0.0f) {
		const float s = 0.5f / std::sqrt(trace + 1.0f);
		return Quaternion(
			0.25f / s,
			(a21 - a12) * s,
			(a02 - a20) * s,
			(a10 - a01) * s
		).normalised();
	} else {
		if (a00 > a11 && a00 > a22) {
			const float s = 2.0f * std::sqrt(1.0f + a00 - a11 - a22);
			return Quaternion(
				(a21 - a12) / s,
				0.25f * s,
				(a01 + a10) / s,
				(a02 + a20) / s
			).normalised();
		} else if (a11 > a22) {
			const float s = 2.0f * std::sqrt(1.0f + a11 - a00 - a22);
			return Quaternion(
				(a02 - a20) / s,
				(a01 + a10) / s,
				0.25f * s,
				(a12 + a21) / s
			).normalised();
		} else {
			const float s = 2.0f * std::sqrt(1.0f + a22 - a00 - a11);
			return Quaternion(
				(a10 - a01) / s,
				(a02 + a20) / s,
				(a12 + a21) / s,
				0.25f * s
			).normalised();
		}
	}
}
