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
#include "../text/halleystring.h"
#include "../support/assert.h"

namespace Halley {
	template <typename T>
	inline T colMinValue()
	{
		return 0;
	}

	template <typename T>
	inline T colMaxValue()
	{
		return 255;
	}

	template <>
	inline float colMaxValue<float>()
	{
		return 1.0f;
	}

	template <typename T, typename U>
	inline U convertColour(T x)
	{
		return U(float(x) * colMaxValue<U>() / colMaxValue<T>());
	}

	template <typename T>
	class Colour4 {
	public:
		T r, g, b, a;

		Colour4()
			: r(colMinValue<T>())
			, g(colMinValue<T>())
			, b(colMinValue<T>())
			, a(colMaxValue<T>())
		{}
		
		Colour4(T _r, T _g, T _b, T _a=colMaxValue<T>())
			: r(_r)
			, g(_g)
			, b(_b)
			, a(_a)
		{}

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
			assert(str[0] == '#');
			if (len >= 3) col.r = parseHex(str.substr(1, 2));
			else col.r = 0;
			if (len >= 5) col.g = parseHex(str.substr(3, 2));
			else col.g = 0;
			if (len >= 7) col.b = parseHex(str.substr(5, 2));
			else col.b = 0;
			if (len >= 9) col.a = parseHex(str.substr(7, 2));
			else col.a = colMaxValue<T>();
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
				h = ::fmod(h, 1);
				int hi = int(h*6);
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

		Colour4 multiplyLuma(float t) const {
			return Colour4(r*t, g*t, b*t, a);
		}

		Colour4 operator+(const Colour4& c) const
		{
			return Colour4(r+c.r, g+c.g, b+c.b, a+c.a);
		}

		Colour4 operator*(float t) const
		{
			return Colour4(r*t, g*t, b*t, a*t);
		}

		bool operator==(const Colour4& c) const
		{
			return r == c.r && g == c.g && b == c.b && a == c.a;
		}

		bool operator!=(const Colour4& c) const
		{
			return r != c.r || g != c.g || b != c.b || a != c.a;
		}

	private:
		unsigned char byteRep(T v) const
		{
			return convertColour<T, unsigned char>(v);
		}

		static T parseHex(String str)
		{
			std::stringstream ss(str);
			int value;
			ss << std::hex;
			ss >> value;
			return convertColour<unsigned char, T>((unsigned char)(value));
		}

		template <typename U>
		void writeByte(U& os, T component) const
		{
			unsigned int value = byteRep(component);
			if (value < 16) os << '0';
			os << value;
		}
	};

	typedef Colour4<unsigned char> Colour4c;
	typedef Colour4<float> Colour4f;
	typedef Colour4f Colour;
}
