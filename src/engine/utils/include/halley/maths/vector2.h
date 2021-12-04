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
#include <ostream>
#include "angle.h"
#include <halley/utils/utils.h>
#include "halley/text/string_converter.h"
#include <gsl/gsl>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace Halley {
	//////////////////////////////
	// Vector2D class declaration
	template <typename T=float, class U=Angle<float>>
	class alignas(sizeof(T)*2) Vector2D {
	private:
		constexpr static T mod(T a, T b) noexcept { return a % b; }

	public:
		using ScalarType = T;
		T x,y;

		// Constructors
		constexpr Vector2D () noexcept : x(0), y(0) {}
		constexpr Vector2D (T x, T y) noexcept : x(x), y(y) {}
		constexpr Vector2D (const Vector2D& vec) noexcept : x(static_cast<T>(vec.x)), y(static_cast<T>(vec.y)) {}
		template <typename V> constexpr explicit Vector2D (const Vector2D<V>& vec) noexcept : x(static_cast<T>(vec.x)), y(static_cast<T>(vec.y)) {}

		constexpr Vector2D (T length, U angle) noexcept
		{
			const float s = angle.sin();
			const float c = angle.cos();
			x = c * length;
			y = s * length;
		}

		// Getter
		constexpr T& operator[](size_t n)
		{
			Expects(n <= 1);
			return (&x)[n];
		}
		constexpr T operator[](size_t n) const
		{
			Expects(n <= 1);
			return (&x)[n];
		}

		// Assignment and comparison
		constexpr Vector2D& operator = (const Vector2D& param) { x = param.x; y = param.y; return *this; }
		constexpr Vector2D& operator = (Vector2D&& param) noexcept { x = param.x; y = param.y; return *this; }
		constexpr Vector2D& operator = (T param) { x = param; y = param; return *this; }
		constexpr bool operator == (Vector2D param) const { return x == param.x && y == param.y; }
		constexpr bool operator != (Vector2D param) const { return x != param.x || y != param.y; }
		constexpr bool operator < (Vector2D param) const { return y != param.y ? y < param.y : x < param.x; }

		// Basic algebra
		constexpr Vector2D operator + (Vector2D param) const { return Vector2D(x + param.x,y + param.y); }
		constexpr Vector2D operator - (Vector2D param) const { return Vector2D(x - param.x,y - param.y); }
		constexpr Vector2D operator * (Vector2D param) const { return Vector2D(x * param.x,y * param.y); }
		constexpr Vector2D operator / (Vector2D param) const { return Vector2D(x / param.x,y / param.y); }
		constexpr Vector2D operator % (Vector2D param) const { return Vector2D(mod(x, param.x), mod(y, param.y)); }

		constexpr Vector2D modulo(Vector2D param) const { return Vector2D(Halley::modulo<T>(x, param.x), Halley::modulo<T>(y, param.y)); }
		constexpr Vector2D floorDiv(Vector2D param) const { return Vector2D(Halley::floorDiv(x, param.x), Halley::floorDiv(y, param.y)); }

		constexpr Vector2D operator - () const { return Vector2D(-x,-y); }

		template <typename V>
		constexpr Vector2D operator * (V param) const { return Vector2D(T(x * param), T(y * param)); }

		template <typename V>
		constexpr Vector2D operator / (V param) const { return Vector2D(T(x / param), T(y / param)); }

		// In-place operations
		constexpr Vector2D& operator += (Vector2D param) { x += param.x; y += param.y; return *this; }
		constexpr Vector2D& operator -= (Vector2D param) { x -= param.x; y -= param.y; return *this; }
		constexpr Vector2D& operator *= (Vector2D param) { x *= param.x; y *= param.y; return *this; }
		constexpr Vector2D& operator /= (Vector2D param) { x /= param.x; y /= param.y; return *this; }
		constexpr Vector2D& operator %= (Vector2D param) { x %= param.x; y %= param.y; return *this; }
		constexpr Vector2D& operator *= (const T param) { x *= param; y *= param; return *this; }
		constexpr Vector2D& operator /= (const T param) { x /= param; y /= param; return *this; }

		// Get the normalized vector (unit vector)
		constexpr Vector2D unit () const
		{
			float len = length();
			if (len != 0) {
				return (*this) / len;
			} else {
				return Vector2D(0, 0);
			}
		}
		constexpr Vector2D normalized() const
		{
			return unit();
		}
		constexpr void normalize()
		{
			*this = unit();
		}

		// Get the orthogonal vector
		constexpr Vector2D orthoLeft () const { return Vector2D(-y, x); }
		constexpr Vector2D orthoRight () const { return Vector2D(y, -x); }

		// Cross product (the Z component of it)
		constexpr T cross (Vector2D param) const { return x * param.y - y * param.x; }

		// Dot product
		constexpr T dot (Vector2D param) const { return (x * param.x) + (y * param.y); }

		// Length
		constexpr T length () const { return static_cast<T>(std::sqrt(squaredLength())); }
		constexpr T len () const { return length(); }
		constexpr T manhattanLength() const { return std::abs(x) + std::abs(y); }

		// Squared length, often useful and much faster
		constexpr T squaredLength () const { return x*x+y*y; }

		// Projection on another vector
		constexpr Vector2D projection (Vector2D param) const
		{
			Vector2D unit = param.unit();
			return this->dot(unit) * unit;
		}
		constexpr T projectionLength (Vector2D param) const
		{
			return this->dot(param.unit());
		}

		// Rounding
		constexpr Vector2D floor() const { return Vector2D(static_cast<T>(std::floor(x)), static_cast<T>(std::floor(y))); }
		constexpr Vector2D ceil() const { return Vector2D(static_cast<T>(std::ceil(x)), static_cast<T>(std::ceil(y))); }
		constexpr Vector2D round() const { return Vector2D(static_cast<T>(std::round(x)), static_cast<T>(std::round(y))); }

		// Gets the angle that this vector is pointing to
		[[nodiscard]] constexpr U angle () const
		{
			U angle;
			angle.setRadians(std::atan2(y, x));
			return angle;
		}

		// Rotates vector by an angle
		[[nodiscard]] constexpr Vector2D rotate (U angle) const
		{
			const T sin = angle.sin();
			const T cos = angle.cos();
			return Vector2D(x*cos - y*sin, x*sin + y*cos);
		}

		// Rotates vector by sine and cosine
		[[nodiscard]] constexpr Vector2D rotate (T sine, T cosine) const
		{
			return Vector2D(x*cosine - y*sine, x*sine + y*cosine);
		}

		// Removes the length of the vector along the given axis
		[[nodiscard]] constexpr Vector2D neutralize (Vector2D param) const
		{
			return *this - dot(param)*param;
		}

		String toString() const
		{
			return String("(") + x + ", " + y + ")";
		}

		constexpr Vector2D abs() const
		{
			return Vector2D(std::abs(x), std::abs(y));
		}

		constexpr bool isValid() const
		{
			return !std::isnan(x) && !std::isnan(y) && !std::isinf(x) && !std::isinf(y);
		}

		bool epsilonEquals(Vector2D other, float epsilon) const
		{
			return std::abs(x - other.x) < epsilon
				&& std::abs(y - other.y) < epsilon;
		}

		constexpr static Vector2D<T,U> min(Vector2D<T,U> a, Vector2D<T,U> b)
		{
			return Vector2D<T,U>(std::min(a.x, b.x), std::min(a.y, b.y));
		}

		constexpr static Vector2D<T,U> max(Vector2D<T,U> a, Vector2D<T,U> b)
		{
			return Vector2D<T,U>(std::max(a.x, b.x), std::max(a.y, b.y));
		}
	};


	////////////////////
	// Global operators
	template <typename T,class U,typename V>
	inline Vector2D<T,U> operator * (V f, const Vector2D<T,U> &v)
	{
		return Vector2D<T,U>(T(v.x * f),T(v.y * f));
	}

	template <typename T,class U>
	std::ostream& operator<< (std::ostream& ostream, const Vector2D<T, U>& v)
	{
		ostream << "(" << v.x << "," << v.y << ")" ; return ostream;
	}
	
	////////////
	// Typedefs
	typedef Vector2D<double> Vector2d;
	typedef Vector2D<> Vector2f;
	typedef Vector2D<int> Vector2i;
	typedef Vector2D<short> Vector2s;
	typedef Vector2D<char> Vector2c;
	typedef Vector2f Position;
	typedef Vector2f Size;
}

namespace std {
	template<>
	struct hash<Halley::Vector2i>
	{
		size_t operator()(const Halley::Vector2i& v) const noexcept
		{
			return std::hash<long long>()(*reinterpret_cast<const long long*>(&v));
		}
	};

	template<>
	struct hash<Halley::Vector2f>
	{
		size_t operator()(const Halley::Vector2f& v) const noexcept
		{
			return std::hash<long long>()(*reinterpret_cast<const long long*>(&v));
		}
	};
}
