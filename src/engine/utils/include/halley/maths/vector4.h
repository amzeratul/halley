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
#include <iostream>
#include "angle.h"
#include "vector2.h"
#include "vector3.h"
#include <halley/utils/utils.h>
#include <gsl/gsl_assert>

namespace Halley {
	//////////////////////////////
	// Vector4D class declaration
	template <typename T=float, int Alignment=4>
	class alignas(Alignment * sizeof(T)) Vector4D {
	private:
		constexpr inline T mod(T a, T b) const { return a % b; }

	public:
		T x, y, z, w;

		// Constructors
		constexpr inline Vector4D ()
			: x(0)
			, y(0)
			, z(0)
			, w(0)
		{}
		
		constexpr inline Vector4D (T x, T y, T z, T w)
			: x(x)
			, y(y)
			, z(z)
			, w(w)
		{}
		
		template <typename V, int A>
		constexpr inline explicit Vector4D (Vector4D<V, A> vec)
			: x(T(vec.x))
			, y(T(vec.y))
			, z(T(vec.z))
			, w(T(vec.w))
		{}

		constexpr inline explicit Vector4D(const Vector2D<T>& v, T z = 0, T w = 0)
			: x(v.x)
			, y(v.y)
			, z(z)
			, w(w)
		{}

		constexpr inline explicit Vector4D(const Vector3D<T>& v, T w = 0)
			: x(v.x)
			, y(v.y)
			, z(v.z)
			, w(w)
		{}
		
		// Getter
		constexpr inline T& operator[](size_t n)
		{
			Expects(n <= 3);
			return (&x)[n];
		}

		constexpr inline const T& operator[](size_t n) const
		{
			Expects(n <= 3);
			return (&x)[n];
		}

		// Assignment and comparison
		constexpr inline Vector4D& operator = (Vector4D param) { x = param.x; y = param.y; z = param.z; w = param.w; return *this; }
		constexpr inline Vector4D& operator = (T param) { x = param; y = param; z = param; w = param; return *this; }
		constexpr inline bool operator == (Vector4D param) const { return x == param.x && y == param.y && z == param.z && w == param.w; }
		constexpr inline bool operator != (Vector4D param) const { return x != param.x || y != param.y || z != param.z || w != param.w; }

		// Basic algebra
		constexpr inline Vector4D operator + (Vector4D param) const { return Vector4D(x + param.x, y + param.y, z + param.z, w + param.w); }
		constexpr inline Vector4D operator - (Vector4D param) const { return Vector4D(x - param.x, y - param.y, z - param.z, w - param.w); }
		constexpr inline Vector4D operator * (Vector4D param) const { return Vector4D(x * param.x, y * param.y, z * param.z, w * param.w); }
		constexpr inline Vector4D operator / (Vector4D param) const { return Vector4D(x / param.x, y / param.y, z / param.z, w / param.w); }
		constexpr inline Vector4D operator % (Vector4D param) const { return Vector4D(mod(x, param.x), mod(y, param.y), mod(z, param.z), mod(w, param.w)); }

		constexpr inline Vector4D modulo(Vector4D param) const { return Vector4D(Halley::modulo<T>(x, param.x), Halley::modulo<T>(y, param.y), Halley::modulo<T>(z, param.z), Halley::modulo<T>(w, param.w)); }
		constexpr inline Vector4D floorDiv(Vector4D param) const { return Vector4D(Halley::floorDiv(x, param.x), Halley::floorDiv(y, param.y), Halley::floorDiv(z, param.z), Halley::floorDiv(w, param.w)); }

		constexpr inline Vector4D operator - () const { return Vector4D(-x, -y, -z, -w); }

		template <typename V>
		constexpr inline Vector4D operator * (V param) const { return Vector4D(T(x * param), T(y * param), T(z * param), T(w * param)); }

		template <typename V>
		constexpr inline Vector4D operator / (V param) const { return Vector4D(T(x / param), T(y / param), T(z * param), T(w * param)); }

		// In-place operations
		constexpr inline Vector4D& operator += (Vector4D param) { x += param.x; y += param.y; z += param.z; w += param.w; return *this; }
		constexpr inline Vector4D& operator -= (Vector4D param) { x -= param.x; y -= param.y; z -= param.z; w -= param.w; return *this; }
		constexpr inline Vector4D& operator *= (const T param) { x *= param; y *= param; z *= param; w *= param; return *this; }
		constexpr inline Vector4D& operator /= (const T param) { x /= param; y /= param; z /= param; w /= param; return *this; }

		// Get the normalized vector (unit vector)
		inline Vector4D unit () const
		{
			float len = length();
			if (len != 0) {
				return (*this) / len;
			} else {
				return Vector4D();
			}
		}

		inline Vector4D normalized() const
		{
			return unit();
		}

		inline void normalize()
		{
			*this = unit();
		}

		// Dot product
		constexpr inline T dot (Vector4D param) const
		{

			return (x * param.x) + (y * param.y) + (z * param.z) + (w * param.w);
		}

		// Length
		constexpr inline T length () const { static_cast<T>(std::sqrt(squaredLength())); }
		constexpr inline T len () const { return length(); }

		// Squared length, often useful and much faster
		constexpr inline T squaredLength () const { return x*x + y*y + z*z + w*w; }

		// Floor
		inline Vector4D floor() const { return Vector4D(floor(x), floor(y), floor(z), floor(w)); }
		inline Vector4D ceil() const { return Vector4D(ceil(x), ceil(y), ceil(z), ceil(w)); }

		constexpr Vector2D<T> toVector2() const
		{
			return Vector2D<T>(x / w, y / w);
		}

		constexpr Vector3D<T> toVector3() const
		{
			return Vector3D<T>(x / w, y / w, z / w);
		}
	};


	////////////////////
	// Global operators
	template <typename T, typename V>
	constexpr inline Vector4D<T> operator * (V f, const Vector4D<T> &v)
	{
		return Vector4D<T>(T(v.x * f), T(v.y * f), T(v.z * f), T(v.w * f));
	}

	template <typename T >
	std::ostream& operator<< (std::ostream& ostream, const Vector4D<T>& v)
	{
		ostream << "(" << v.x << "," << v.y << "," << v.z << "," << v.w << ")" ; return ostream;
	}

	////////////
	// Typedefs
	using Vector4d = Vector4D<double, 4>;
	using Vector4f = Vector4D<float, 4>;
	using Vector4i = Vector4D<int, 4>;
	using Vector4s = Vector4D<short, 4>;
	using Vector4c = Vector4D<char, 4>;
}
