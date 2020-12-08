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

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include "halley/text/halleystring.h"
#include <gsl/gsl_assert>
#include <cstdint>

namespace Halley {
	// This whole class is TERRIBLE
	// There should be a type distinction between linear and gamma space, with conversions,
	// and the default Colour type should be Colour4c
	
	template <typename T>
	constexpr T colMinValue()
	{
		return 0;
	}

	template <typename T>
	constexpr T colMaxValue()
	{
		return 255;
	}

	template <>
	constexpr float colMaxValue<float>()
	{
		return 1.0f;
	}

	template <typename T, typename U>
	constexpr U convertColour(T x)
	{
		return U(float(x) * colMaxValue<U>() / colMaxValue<T>());
	}

	template <>
	constexpr uint8_t convertColour(int x)
	{
		return static_cast<uint8_t>(x);
	}

	template <typename T>
	class Colour4 {
	public:
		T r = colMinValue<T>();
		T g = colMinValue<T>();
		T b = colMinValue<T>();
		T a = colMaxValue<T>();

		constexpr Colour4() = default;

		constexpr Colour4(T luma)
			: r(luma)
			, g(luma)
			, b(luma)
		{}
		
		constexpr Colour4(T r, T g, T b, T a=colMaxValue<T>())
			: r(r)
			, g(g)
			, b(b)
			, a(a)
		{}

		Colour4(const String& str)
		{
			*this = fromString(str);
		}

		template <typename U>
		Colour4(const Colour4<U> c)
		{
			r = convertColour<U, T>(c.r);
			g = convertColour<U, T>(c.g);
			b = convertColour<U, T>(c.b);
			a = convertColour<U, T>(c.a);
		}

		String toString() const
		{
			std::stringstream ss;
			ss << "#" << std::hex;
			writeByte(ss, r);
			writeByte(ss, g);
			writeByte(ss, b);
			if (byteRep(a) != 255) writeByte(ss, a);
			ss.flush();
			return ss.str();
		}

		static Colour4 fromString(String str)
		{
			Colour4 col;
			size_t len = str.length();
			if (len >= 1 && str[0] == '#') {
				if (len >= 3) {
					col.r = parseHex(str.substr(1, 2));
				}
				if (len >= 5) {
					col.g = parseHex(str.substr(3, 2));
				}
				if (len >= 7) {
					col.b = parseHex(str.substr(5, 2));
				}
				if (len >= 9) {
					col.a = parseHex(str.substr(7, 2));
				}
			}
			return col;
		}

		static Colour4 fromHSV(float h, float s, float v)
		{
			float r = 0;
			float g = 0;
			float b = 0;
			if (s == 0) {
				r = v * 255;
				if (r < 0) r = 0;
				if (r > 255) r = 255;
				g = b = r;
			} else {
				h = float(std::fmod(h, 1.0f));
				int hi = int(h * 6.0f);
				float f = h*6 - hi;
				float p = v*(1-s);
				float q = v*(1-f*s);
				float t = v*(1-(1-f)*s);

				switch (hi) {
				case 0:	r = v; g = t; b = p; break;
				case 1:	r = q; g = v; b = p; break;
				case 2:	r = p; g = v; b = t; break;
				case 3:	r = p; g = q; b = v; break;
				case 4:	r = t; g = p; b = v; break;
				case 5:	r = v; g = p; b = q; break;
				//default: throw Exception("Invalid value!");
				}
			}

			return Colour4<T>(Colour4<float>(r, g, b));
		}

		constexpr Colour4 multiplyLuma(float t) const
		{
			return Colour4(r*t, g*t, b*t, a);
		}

		constexpr Colour4 inverseMultiplyLuma(float t) const
		{
			return Colour4(1.0f - ((1.0f - r) * t), 1.0f - ((1.0f - g) * t), 1.0f - ((1.0f - b) * t), a);
		}

		constexpr Colour4 multiplyAlpha(float t) const
		{
			return Colour4(r, g, b, a * t);
		}

		constexpr Colour4 operator+(const Colour4& c) const
		{
			return Colour4(r+c.r, g+c.g, b+c.b, a+c.a);
		}

		constexpr Colour4 operator*(const Colour4& c) const
		{
			return Colour4(r * c.r, g * c.g, b * c.b, a * c.a);
		}

		constexpr Colour4 operator*(float t) const
		{
			return Colour4(r*t, g*t, b*t, a*t);
		}

		constexpr bool operator==(const Colour4& c) const
		{
			return r == c.r && g == c.g && b == c.b && a == c.a;
		}

		constexpr bool operator!=(const Colour4& c) const
		{
			return r != c.r || g != c.g || b != c.b || a != c.a;
		}

	private:
		uint8_t byteRep(T v) const
		{
			return convertColour<T, uint8_t>(v);
		}

		static T parseHex(String str)
		{
			std::stringstream ss(str);
			int value;
			ss << std::hex;
			ss >> value;
			return convertColour<uint8_t, T>(static_cast<uint8_t>(value));
		}

		template <typename U>
		void writeByte(U& os, T component) const
		{
			unsigned int value = byteRep(component);
			if (value < 16) os << '0';
			os << value;
		}
	};

	typedef Colour4<uint8_t> Colour4c;
	typedef Colour4<float> Colour4f;
	typedef Colour4f Colour;
}
