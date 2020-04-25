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
		constexpr Angle() : value(0) {}
		constexpr Angle(T _value) : value(_value) {}
		constexpr Angle(const Angle &angle) : value(angle.value) {}
		constexpr Angle(Angle&& angle) noexcept : value(std::move(angle.value)) {}

		// Comparison
		constexpr bool operator== (const Angle &param) const { return value == param.value; }
		constexpr bool operator!= (const Angle &param) const { return value != param.value; }
		constexpr bool operator< (const Angle &param) const { return value < param.value; }
		constexpr bool operator<= (const Angle &param) const { return value <= param.value; }
		constexpr bool operator> (const Angle &param) const { return value > param.value; }
		constexpr bool operator>= (const Angle &param) const { return value >= param.value; }

		// Basic arithmetics
		constexpr Angle operator+ (const Angle &param) const
		{
			Angle final;
			final.value = value + param.value;
			final.limit();
			return final;
		}
		constexpr Angle operator- (const Angle &param) const
		{
			Angle final;
			final.value = value - param.value;
			final.limit();
			return final;
		}
		constexpr Angle operator- () const
		{
			Angle final;
			final.value = -value;
			final.limit();
			return final;
		}

		// In-place operations
		constexpr Angle& operator= (const Angle &angle) noexcept { value = angle.value; return *this; }
		constexpr Angle& operator= (Angle&& angle) noexcept { value = angle.value; return *this; }
		constexpr void operator+= (const Angle &angle) { value += angle.value; limit(); }
		constexpr void operator-= (const Angle &angle) { value -= angle.value; limit(); }

		// Accessors
		constexpr void setDegrees(const T degrees) { value = degToRad(degrees); limit(); }
		constexpr void setRadians(const T radian) { value = radian; limit();}
		constexpr T getDegrees() const { return radToDeg(value); }
		constexpr T getRadians() const { return value; }
		constexpr T toDegrees() const { return radToDeg(value); }
		constexpr T toRadians() const { return value; }

		// Which side should it turn to to reach the parameter angle?
		constexpr T turnSide(const Angle &param) const
		{
			float res = std::sin(param.value - value);
			if (res > 0.0f) return 1.0f;
			else return -1.0f;
		}

		// Turns a specific number of radians towards a target
		constexpr void turnRadiansTowards(const Angle &angle,const T radians)
		{
			float side = turnSide(angle);
			value += radians * side;
			limit();
			float newSide = turnSide(angle);
			if (side != newSide) value = angle.value;
		}

		// Turns a specific number of degrees towards a target
		constexpr void turnDegreesTowards(const Angle &angle,const T degrees) { turnRadiansTowards(angle,degToRad(degrees)); }

		// Distance to another angle
		constexpr Angle distance(const Angle &param) const
		{
			float v = std::fabs(value - param.value);
			if (v > PI_CONSTANT_F) v = 2.0f*PI_CONSTANT_F - v;
			return fromRadians(v);
		}

		// Conversion
		constexpr static T degToRad(const T degrees) noexcept { return degrees * 0.01745329252f; }
		constexpr static T radToDeg(const T radians) noexcept { return radians * 57.295779513f; }

		// Trigonometric functions
		constexpr T sin() const noexcept { return std::sin(value); }
		constexpr T cos() const noexcept { return std::cos(value); }
		constexpr T tan() const noexcept { return std::tan(value); }

		// Builder methods
		constexpr static Angle fromRadians (const T radians) noexcept { Angle ang; ang.setRadians(radians); return ang; }
		constexpr static Angle fromDegrees (const T degrees) noexcept { Angle ang; ang.setDegrees(degrees); return ang; }

	private:
		// Angle value in radians
		T value;

		constexpr void limit()
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
