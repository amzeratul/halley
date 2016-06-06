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

#ifndef PI_CONSTANT
#define PI_CONSTANT 3.1415926535897932384626433832795
#define PI_CONSTANT_F 3.1415926535897932384626433832795f
#endif

#include <cmath>
#include <halley/utils/utils.h>

namespace Halley {
	///////////////
	// Angle class
	template <typename T>
	class Angle {
	public:
		// Constructors
		Angle () : value(0) {}
		Angle (T _value) : value(_value) {}
		Angle (const Angle &angle) : value(angle.value) {}

		// Comparison
		inline bool operator== (const Angle &param) const { return value == param.value; }
		inline bool operator!= (const Angle &param) const { return value != param.value; }
		inline bool operator< (const Angle &param) const { return value < param.value; }
		inline bool operator<= (const Angle &param) const { return value <= param.value; }
		inline bool operator> (const Angle &param) const { return value > param.value; }
		inline bool operator>= (const Angle &param) const { return value >= param.value; }

		// Basic arithmetics
		inline Angle operator+ (const Angle &param) const
		{
			Angle final;
			final.value = value + param.value;
			final.limit();
			return final;
		}
		inline Angle operator- (const Angle &param) const
		{
			Angle final;
			final.value = value - param.value;
			final.limit();
			return final;
		}
		inline Angle operator- () const
		{
			Angle final;
			final.value = -value;
			final.limit();
			return final;
		}

		// In-place operations
		inline Angle& operator= (const Angle &angle) { value = angle.value; return *this; }
		inline void operator+= (const Angle &angle) { value += angle.value; limit(); }
		inline void operator-= (const Angle &angle) { value -= angle.value; limit(); }

		// Accessors
		inline void setDegrees(const T degrees) { value = degToRad(degrees); limit(); }
		inline void setRadians(const T radian) { value = radian; limit();}
		inline T getDegrees() const { return radToDeg(value); }
		inline T getRadians() const { return value; }
		inline T toDegrees() const { return radToDeg(value); }
		inline T toRadians() const { return value; }

		// Which side should it turn to to reach the parameter angle?
		inline T turnSide(const Angle &param) const
		{
			float res = ::sin(param.value - value);
			if (res > 0.0f) return 1.0f;
			else return -1.0f;
		}

		// Turns a specific number of radians towards a target
		inline void turnRadiansTowards(const Angle &angle,const T radians)
		{
			float side = turnSide(angle);
			value += radians * side;
			limit();
			float newSide = turnSide(angle);
			if (side != newSide) value = angle.value;
		}

		// Turns a specific number of degrees towards a target
		inline void turnDegreesTowards(const Angle &angle,const T degrees) { turnRadiansTowards(angle,degToRad(degrees)); }

		// Distance to another angle
		inline Angle distance(const Angle &param) const
		{
			float v = fabs(value - param.value);
			if (v > PI_CONSTANT_F) v = 2.0f*PI_CONSTANT_F - v;
			return fromRadians(v);
		}

		// Conversion
		static inline T degToRad(const T degrees) { return degrees * 0.01745329252f; }
		static inline T radToDeg(const T radians) { return radians * 57.295779513f; }

		// Trigonometric functions
		inline T sin() const { return ::sin(value); }
		inline T cos() const { return ::cos(value); }
		inline T tan() const { return ::tan(value); }
		inline void sincos(T& s, T& c) const {
#if defined(_MSC_VER) && defined(_M_IX86)
			float _v = value;
			float _s;
			float _c;
			__asm {
				fld _v
				fsincos
				fstp _c
				fstp _s
			}
			s = _s;
			c = _c;
#else
			s = ::sin(value);
			c = ::cos(value);
#endif
		}

		// Builder methods
		inline static Angle fromRadians (const T radians) { Angle ang; ang.setRadians(radians); return ang; }
		inline static Angle fromDegrees (const T radians) { Angle ang; ang.setDegrees(radians); return ang; }

	private:
		// Angle value in radians
		T value;

		inline void limit()
		{
			value = modulo(value, 2*PI_CONSTANT_F);
		}
	};

	template <typename T>
	Angle<T> operator* (const Angle<T>& a, float b) { return Angle<T>(a.getRadians() * b); }

	template <typename T>
	Angle<T> operator* (float a, const Angle<T>& b) { return Angle<T>(b.getRadians() * a); }

	// Typedefs
	typedef Angle<double> Angle1d;
	typedef Angle<float> Angle1f;
}
