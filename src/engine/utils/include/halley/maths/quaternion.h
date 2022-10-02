#pragma once

#include "vector3.h"
#include "halley/text/string_converter.h"

namespace Halley {
	class Matrix4f;

	class alignas(16) Quaternion {
	public:
		float w = 1, x = 0, y = 0, z = 0;

		constexpr Quaternion() = default;
		constexpr Quaternion(const Quaternion& other) = default;
		constexpr Quaternion(Quaternion&& other) noexcept = default;
		constexpr Quaternion(float w, float x, float y, float z)
			: w(w), x(x), y(y), z(z)
		{}
		Quaternion(Vector3f axis, Angle1f angle);
		Quaternion(Vector3f v);

		constexpr Quaternion& operator=(const Quaternion& other) = default;
		constexpr Quaternion& operator=(Quaternion&& other) noexcept = default;

		constexpr Quaternion operator+(const Quaternion& other) const
		{
			return Quaternion(w + other.w, x + other.x, y + other.y, z + other.z);
		}
		constexpr Quaternion operator-(const Quaternion& other) const
		{
			return Quaternion(w - other.w, x - other.x, y - other.y, z - other.z);
		}
		constexpr Quaternion operator-() const
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

		[[nodiscard]] String toString() const
		{
			return String("(w:") + w + ", x:" + x + ", y:" + y + ", z:" + z + ")";
		}
	};
}
