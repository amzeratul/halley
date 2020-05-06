/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/


#pragma once

#include <cmath>
#include "vector2.h"

namespace Halley {
	//////////////////////////////
	// Vector3D class declaration
	template <typename T=float>
	class alignas(16) Vector3D {
	private:
		constexpr T Mod(T a, T b) const { return a % b; }

	public:
		T x, y, z;

	private:
		T pad = 0;

	public:
		// Constructors
		constexpr Vector3D () { x = y = z = 0; }
		constexpr Vector3D (T x, T y, T z) { this->x = x; this->y = y; this->z = z; }
		constexpr explicit Vector3D (Vector2D<T> vec, T z = 0) { x = vec.x; y = vec.y; this->z = z; }
		constexpr Vector3D (const Vector3D<T> &vec) { x = vec.x; y = vec.y; z = vec.z; }
		template <typename V>
		constexpr explicit Vector3D (const Vector3D<V> &vec) { x = T(vec.x); y = T(vec.y); z = T(vec.z); }

		// Getter
		constexpr inline T& operator[](size_t n)
		{
			Expects(n <= 2);
			return (&x)[n];
		}
		constexpr inline T operator[](size_t n) const
		{
			Expects(n <= 2);
			return (&x)[n];
		}

		// Assignment and comparison
		constexpr inline void operator = (const Vector3D &p) { x = p.x; y = p.y; z = p.z; }
		constexpr inline void operator = (const T p) { x = p; y = p; z = p; }
		constexpr inline bool operator == (const Vector3D &p) const { return x == p.x && y == p.y && z == p.z; }
		constexpr inline bool operator != (const Vector3D &p) const { return x != p.x || y != p.y || z != p.z; }

		// Basic algebra
		constexpr inline Vector3D operator + (const Vector3D &p) const { return Vector3D(x + p.x,y + p.y,z + p.z); }
		constexpr inline Vector3D operator - (const Vector3D &p) const { return Vector3D(x - p.x,y - p.y,z - p.z); }
		constexpr inline Vector3D operator * (const Vector3D &p) const { return Vector3D(x * p.x,y * p.y,z * p.z); }
		constexpr inline Vector3D operator / (const Vector3D &p) const { return Vector3D(x / p.x,y / p.y,z / p.z); }
		constexpr inline Vector3D operator % (const Vector3D &p) const { return Vector3D(Mod(x, p.x), Mod(y, p.y), Mod(z, p.z)); }

		inline Vector3D modulo(const Vector3D &p) const { return Vector3D(Halley::modulo<T>(x, p.x), Halley::modulo<T>(y, p.y), Halley::modulo<T>(z, p.z)); }
		inline Vector3D floorDiv(const Vector3D &p) const { return Vector3D(Halley::floorDiv(x, p.x), Halley::floorDiv(y, p.y), Halley::floorDiv(z, p.z)); }

		constexpr inline Vector3D operator - () const { return Vector3D(-x, -y, -z); }

		constexpr inline Vector3D operator * (const float p) const { return Vector3D(T(x * p), T(y * p), T(z * p)); }
		constexpr inline Vector3D operator / (const T p) const { return Vector3D(x / p,y / p, z / p); }

		// In-place operations
		constexpr inline Vector3D operator += (const Vector3D &p) { x += p.x; y += p.y; z += p.z; return *this; }
		constexpr inline Vector3D operator -= (const Vector3D &p) { x -= p.x; y -= p.y; z -= p.z; return *this; }
		constexpr inline Vector3D operator *= (const T p) { x *= p; y *= p; z *= p; return *this; }
		constexpr inline Vector3D operator /= (const T p) { x /= p; y /= p; z /= p; return *this; }

		// Get the normalized vector (unit vector)
		inline Vector3D unit() const
		{
			float len = length();
			if (len != 0) {
				return (*this) * (1.0f / len);
			}
			return Vector3D(T(0), T(0), T(0));
		}

		// Cross product
		constexpr inline Vector3D cross(const Vector3D& p) const { return Vector3D(y * p.z - z * p.y, z * p.x - x * p.z, x * p.y - y * p.x); }

		// Dot product
		constexpr inline T dot(const Vector3D& p) const { return (x * p.x) + (y * p.y) + (z * p.z); }

		// Length
		constexpr inline T length() const { return static_cast<T>(std::sqrt(squaredLength())); }

		// Squared length, often useful and much faster
		constexpr inline T squaredLength() const { return x * x + y * y + z * z; }

		// Floor
		inline Vector3D floor() const { return Vector3D(::floor(x), ::floor(y), ::floor(z)); }

		// Projection on another vector
		inline Vector3D projection(const Vector3D &p) const { Vector3D unit = p.unit(); return dot(unit) * unit; }
		inline T projectionLength(const Vector3D &p) const { Vector3D unit = p.unit(); return dot(unit); }

		inline Vector3D normalized() const
		{
			return unit();
		}
		inline void normalize()
		{
			*this = unit();
		}

		String toString() const
		{
			return String("(") + x + ", " + y + ", " + z + ")";
		}

		constexpr Vector2D<T> xy() const
		{
			return Vector2D<T>(x, y);
		}
	};


	////////////////////
	// Global operators
	template <typename T,typename V> inline Vector3D<T> operator * (V f,const Vector3D<T> &v) { return v * f; }

	////////////
	// Typedefs
	typedef Vector3D<double> Vector3d;
	typedef Vector3D<float> Vector3f;
	typedef Vector3D<int> Vector3i;
	typedef Vector3D<short> Vector3s;
	typedef Vector3D<char> Vector3c;
}
