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
#include "vector2d.h"

namespace Halley {
	//////////////////////////////
	// Vector3D class declaration
	template <typename T=float>
	class Vector3D {
	private:
		T Mod(T a, T b) const { return a % b; }

	public:
		T x,y,z;

	private:
		T pad = 0;

	public:
		// Constructors
		Vector3D () { x = y = z = 0; }
		Vector3D (const T _x, const T _y, const T _z) { x = _x; y = _y; z = _z; }
		explicit Vector3D (const Vector2D<T> &vec, const T _z=0) { x = vec.x; y = vec.y; z = _z; }
		Vector3D (const Vector3D<T> &vec) { x = vec.x; y = vec.y; z = vec.z; }
		template <typename V>
		explicit Vector3D (const Vector3D<V> &vec) { x = T(vec.x); y = T(vec.y); z = T(vec.z); }

		// Getter
		inline T& operator[](int n) { return *((&x)+n); }
		inline const T& operator[](int n) const { return *((&x) + n); }

		// Assignment and comparison
		inline void operator = (const Vector3D &p) { x = p.x; y = p.y; z = p.z; }
		inline void operator = (const T p) { x = p; y = p; z = p; }
		inline bool operator == (const Vector3D &p) const { return x == p.x && y == p.y && z == p.z; }
		inline bool operator != (const Vector3D &p) const { return x != p.x || y != p.y || z != p.z; }

		// Basic algebra
		inline Vector3D operator + (const Vector3D &p) const { return Vector3D(x + p.x,y + p.y,z + p.z); }
		inline Vector3D operator - (const Vector3D &p) const { return Vector3D(x - p.x,y - p.y,z - p.z); }
		inline Vector3D operator * (const Vector3D &p) const { return Vector3D(x * p.x,y * p.y,z * p.z); }
		inline Vector3D operator / (const Vector3D &p) const { return Vector3D(x / p.x,y / p.y,z / p.z); }
		inline Vector3D operator % (const Vector3D &p) const { return Vector3D(Mod(x, p.x), Mod(y, p.y), Mod(z, p.z)); }

		inline Vector3D modulo(const Vector3D &p) const { return Vector3D(Halley::modulo<T>(x, p.x), Halley::modulo<T>(y, p.y), Halley::modulo<T>(z, p.z)); }
		inline Vector3D floorDiv(const Vector3D &p) const { return Vector3D(Halley::floorDiv(x, p.x), Halley::floorDiv(y, p.y), Halley::floorDiv(z, p.z)); }

		inline Vector3D operator - () const { return Vector3D(-x,-y,-z); }

		inline Vector3D operator * (const float p) const { return Vector3D(T(x * p), T(y * p), T(z * p)); }
		inline Vector3D operator / (const T p) const { return Vector3D(x / p,y / p, z / p); }

		// In-place operations
		inline Vector3D operator += (const Vector3D &p) { x += p.x; y += p.y; z += p.z; return *this; }
		inline Vector3D operator -= (const Vector3D &p) { x -= p.x; y -= p.y; z -= p.z; return *this; }
		inline Vector3D operator *= (const T p) { x *= p; y *= p; z *= p; return *this; }
		inline Vector3D operator /= (const T p) { x /= p; y /= p; z /= p; return *this; }

		// Get the normalized vector (unit vector)
		inline Vector3D getUnit () const
		{
			float len = length();
			if (len != 0) {
				return (*this) / len;
			}
			else return Vector3D(T(0), T(0), T(0));
		}

		// Cross product (the Z component of it)
		// TODO: lazy atm
		//inline Vector3D Cross (const Vector3D &p) const { return x * p.y - y * p.x; }

		// Dot product
		inline T dot (const Vector3D &p) const { return (x * p.x) + (y * p.y) + (z * p.z); }

		// Length
		inline T length () const { return sqrt(squaredLength()); }

		// Squared length, often useful and much faster
		inline T squaredLength () const { return x*x+y*y+z*z; }

		// Floor
		inline Vector3D floor () const { return Vector3D(::floor(x), ::floor(y), ::floor(z)); }

		// Projection on another vector
		inline Vector3D projection (const Vector3D &p) const { Vector3D unit = p.getUnit(); return dot(unit) * unit; }
		inline T projectionLength (const Vector3D &p) const { Vector3D unit = p.getUnit(); return dot(unit); }
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
