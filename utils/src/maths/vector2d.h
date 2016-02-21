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
#include "utils.h"

namespace Halley {
	//////////////////////////////
	// Vector2D class declaration
	template <typename T=float,class U=Angle<float> >
	class Vector2D {
	private:
		T mod(T a, T b) const { return a % b; }

	public:
		T x,y;

		// Constructors
		Vector2D () : x(0), y(0) {}
		
		Vector2D (T _x, T _y) : x(_x), y(_y) {}
		
		template <typename V>
		explicit Vector2D (Vector2D<V> vec) : x((T)vec.x), y((T)vec.y) {}

		//template <>
		//Vector2D (Vector2D<T> vec) : x(vec.x), y(vec.y) {}

		Vector2D (const T length, const U &angle)
		{
			float s, c;
			angle.sincos(s, c);
			x = s*length;
			y = c*length;
		}

		// Getter
		inline T& operator[](int n) { return n==0? x: y; }
		inline const T& operator[](int n) const { return n==0? x: y; }

		// Assignment and comparison
		inline Vector2D& operator = (const Vector2D &param) { x = param.x; y = param.y; return *this; }
		inline Vector2D& operator = (const T param) { x = param; y = param; return *this; }
		inline bool operator == (const Vector2D &param) const { return x == param.x && y == param.y; }
		inline bool operator != (const Vector2D &param) const { return x != param.x || y != param.y; }

		// Basic algebra
		inline const Vector2D operator + (const Vector2D &param) const { return Vector2D(x + param.x,y + param.y); }
		inline const Vector2D operator - (const Vector2D &param) const { return Vector2D(x - param.x,y - param.y); }
		inline const Vector2D operator * (const Vector2D &param) const { return Vector2D(x * param.x,y * param.y); }
		inline const Vector2D operator / (const Vector2D &param) const { return Vector2D(x / param.x,y / param.y); }
		inline const Vector2D operator % (const Vector2D &param) const { return Vector2D(mod(x, param.x), mod(y, param.y)); }

		inline const Vector2D modulo(const Vector2D &param) const { return Vector2D(Halley::modulo<T>(x, param.x), Halley::modulo<T>(y, param.y)); }
		inline const Vector2D floorDiv(const Vector2D &param) const { return Vector2D(Halley::floorDiv(x, param.x), Halley::floorDiv(y, param.y)); }

		inline const Vector2D operator - () const { return Vector2D(-x,-y); }

		template <typename V>
		inline const Vector2D operator * (const V param) const { return Vector2D(T(x * param), T(y * param)); }

		template <typename V>
		inline const Vector2D operator / (const V param) const { return Vector2D(T(x / param), T(y / param)); }

		// In-place operations
		inline Vector2D& operator += (const Vector2D &param) { x += param.x; y += param.y; return *this; }
		inline Vector2D& operator -= (const Vector2D &param) { x -= param.x; y -= param.y; return *this; }
		inline Vector2D& operator *= (const T param) { x *= param; y *= param; return *this; }
		inline Vector2D& operator /= (const T param) { x /= param; y /= param; return *this; }

		// Get the normalized vector (unit vector)
		inline Vector2D unit () const
		{
			float len = length();
			if (len != 0) {
				return (*this) / len;
			}
			else return Vector2D(0,0);
		}

		// Get the orthogonal vector
		inline const Vector2D orthoLeft () const { return Vector2D(-y,x); }
		inline const Vector2D orthoRight () const { return Vector2D(y,-x); }

		// Cross product (the Z component of it)
		inline T cross (const Vector2D &param) const { return x * param.y - y * param.x; }

		// Dot product
		inline T dot (const Vector2D &param) const { return (x * param.x) + (y * param.y); }

		// Length
		inline T length () const { return sqrt(squaredLength()); }
		inline T len () const { return length(); }

		// Squared length, often useful and much faster
		inline T squaredLength () const { return x*x+y*y; }

		// Projection on another vector
		inline const Vector2D projection (const Vector2D &param) const { Vector2D unit = param.unit(); return dot(unit) * unit; }
		inline T projectionLength (const Vector2D &param) const { Vector2D unit = param.unit(); return dot(unit); }

		// Floor
		inline const Vector2D floor() const { return Vector2D(floor(x), floor(y)); }
		inline const Vector2D ceil() const { return Vector2D(ceil(x), ceil(y)); }

		// Gets the angle that this vector is pointing to
		inline U angle () const
		{
			U angle;
			angle.setRadians(atan2(y,x));
			return angle;
		}

		// Rotates vector by an angle
		inline const Vector2D rotate (const U angle) const
		{
			T cos, sin;
			angle.sincos(sin, cos);
			return Vector2D(x*cos - y*sin, x*sin + y*cos);
		}

		// Rotates vector by sine and cosine
		inline const Vector2D rotate (const T sine,const T cosine) const
		{
			return Vector2D(x*cosine - y*sine, x*sine + y*cosine);
		}

		// Removes the length of the vector along the given axis
		inline const Vector2D neutralize (const Vector2D& param) const { return *this - dot(param)*param; }

		// Boost serialization
		/*
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & x;
			ar & y;
		}
		*/
	};


	////////////////////
	// Global operators
	template <typename T,class U,typename V>
	inline const Vector2D<T,U> operator * (V f, const Vector2D<T,U> &v)
	{
		return Vector2D<T,U>(T(v.x * f),T(v.y * f));
	}
	
	template <typename T,class U,typename V>
	inline const Vector2D<T,U> operator / (V f, const Vector2D<T,U> &v)
	{
		return Vector2D<T,U>(T(f / v.x),T(f / v.y));
	}

	template <typename T,class U>
	std::ostream& operator<< (std::ostream& ostream, const Vector2D<T, U>& v)
	{
		ostream << "(" << v.x << "," << v.y << ")" ; return ostream;
	}

	////////////
	// Typedefs
	typedef Vector2D<double,Angle<double> > Vector2d;
	typedef Vector2D<> Vector2f;
	typedef Vector2D<int> Vector2i;
	typedef Vector2D<short> Vector2s;
	typedef Vector2D<char> Vector2c;
	typedef Vector2f Position;
	typedef Vector2f Size;
}
