#pragma once

#include "vector3.h"

namespace Halley {
	class alignas(16) Quaternion {
	public:
		float a = 1, b = 0, c = 0, d = 0;

		Quaternion() = default;
		Quaternion(const Quaternion& other) = default;
		Quaternion(Quaternion&& other) = default;
		Quaternion(float a, float b, float c, float d)
			: a(a), b(b), c(c), d(d)
		{}
		Quaternion(Vector3f axis, Angle1f angle);

		Quaternion& operator=(const Quaternion& other) = default;
		Quaternion& operator=(Quaternion&& other) = default;

		Quaternion operator+(const Quaternion& other) const
		{
			return Quaternion(a + other.a, b + other.b, c + other.c, d + other.d);
		}
		Quaternion operator-(const Quaternion& other) const
		{
			return Quaternion(a - other.a, b - other.b, c - other.c, d - other.d);
		}
		Quaternion operator-() const
		{
			return Quaternion(-a, -b, -c, -d);
		}
		Quaternion operator*(const Quaternion& other) const;
		Vector3f operator*(Vector3f other) const;

		void loadIdentity();
		void normalise();
		Quaternion normalised() const;

		Quaternion inverse() const;
		Quaternion conjugated() const;
	};
}
