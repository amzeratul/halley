#pragma once

#include "vector3.h"

namespace Halley {
	class Matrix4f;

	class alignas(16) Quaternion {
	public:
		float w = 1, x = 0, y = 0, z = 0;

		Quaternion() = default;
		Quaternion(const Quaternion& other) = default;
		Quaternion(Quaternion&& other) = default;
		Quaternion(float w, float x, float y, float z)
			: w(w), x(x), y(y), z(z)
		{}
		Quaternion(Vector3f axis, Angle1f angle);
		Quaternion(Vector3f v);

		Quaternion& operator=(const Quaternion& other) = default;
		Quaternion& operator=(Quaternion&& other) = default;

		Quaternion operator+(const Quaternion& other) const
		{
			return Quaternion(w + other.w, x + other.x, y + other.y, z + other.z);
		}
		Quaternion operator-(const Quaternion& other) const
		{
			return Quaternion(w - other.w, x - other.x, y - other.y, z - other.z);
		}
		Quaternion operator-() const
		{
			return Quaternion(-w, -x, -y, -z);
		}
		Quaternion operator*(const Quaternion& other) const;
		Vector3f operator*(Vector3f other) const;

		bool operator==(const Quaternion& other) const;
		bool operator!=(const Quaternion& other) const;

		void loadIdentity();
		void normalise();
		Quaternion normalised() const;

		Quaternion inverse() const;
		Quaternion conjugated() const;
		Vector3f toVector3f() const;

		static Quaternion lookAt(const Vector3f& dir, const Vector3f& worldUp);
	};
}
