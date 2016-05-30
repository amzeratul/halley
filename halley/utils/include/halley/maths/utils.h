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

#include <algorithm>
#include <limits>
#include <list>
#include <cmath>

#include <memory>
#include <array>
#include <functional>
#include <vector>


namespace Halley {
	// General aliases
	using Byte = unsigned char;
	using Bytes = std::vector<Byte>;
	
	template <typename T>
	inline T clamp(T value, T minValue, T maxValue)
	{
		return std::min(std::max(minValue, value), maxValue);
	}

	template <typename T>
	inline T maxAbs(T a, T b)
	{
		return abs(a) > abs(b) ? a : b;
	}

	template <typename T>
	inline T minAbs(T a, T b)
	{
		return abs(a) < abs(b) ? a : b;
	}

	template <typename T>
	inline bool rangeIntersection(T s1, T e1, T s2, T e2)
	{
		return (s1 < e2) && (s2 < e1);
	}


	// Endianness conversion
	//const bool IS_BIG_ENDIAN = *((short*)"AB") == 0x4243;
	constexpr bool IS_BIG_ENDIAN = false;

	template <typename T>
	inline T FixEndian(T t)
	{
		if (IS_BIG_ENDIAN) {
			T temp = t;
			unsigned char *v = reinterpret_cast<unsigned char*>(&temp);
			for (size_t i=0; i<sizeof(T)/2; i++) {
				std::swap(v[i], v[sizeof(T)-1-i]);
			}
			return temp;
		} else {
			return t;
		}
	}

	template<>
	inline char FixEndian<char>(char t) {
		return t;
	}

	template<>
	inline unsigned char FixEndian<unsigned char>(unsigned char t) {
		return t;
	}

	// True modulo definition
	template <typename T> inline T modulo (T a, T b)
	{
		T res = a % b;
		if (res < 0) res = b+res;
		return res;
	}

	template <typename T> inline T floatModulo (T a, T b)
	{
		if (b == 0) return a;
		return a - b * floor(a/b);
	}
	template <> inline float modulo (float a, float b) { return floatModulo(a, b); }
	template <> inline double modulo (double a, double b) { return floatModulo(a, b); }

	// Float floor division (e.g. -0.5 / 2 = -1.0)
	template <typename T>
	inline T floorDivFloat (T a, T b)
	{
		return floor(a / b);
	}

	// Int floor division (e.g. -2 / 3 = -1)
	template <typename T>
	inline T floorDivInt (T a, T b)
	{
		T result = a / b;
		if (a % b < 0) {
			--result;
		}
		return result;
	}

	// Generic floor divs
	inline float floorDiv (float a, float b) { return floorDivFloat(a, b); }
	inline double floorDiv (double a, double b) { return floorDivFloat(a, b); }
	inline long floorDiv (long a, long b) { return floorDivInt(a, b); }
	inline int floorDiv (int a, int b) { return floorDivInt(a, b); }
	inline short floorDiv (short a, short b) { return floorDivInt(a, b); }
	inline char floorDiv (char a, char b) { return floorDivInt(a, b); }

	// Interpolation
	template <typename T>
	inline T interpolate(T a, T b, float factor) {
		return T(a*(1-factor) + b*factor);
	}

	// Smoothing
	template <typename T>
	inline T smoothCos(T a) {
		return T((1-cos(a * 3.1415926535897932384626433832795))*0.5);
	}

	// ASR (attack-sustain-release) envelope
	// Returns 0 at start of attack and end of release, 1 during sustain
	// Attack and release are linear interpolations
	template <typename T>
	inline T asr(T x, T a, T s, T r) {
		if (x < a) return x/a;
		T as = a+s;
		if (x < as) return 1;
		return 1 - ((x-as)/r);
	}

	// Next power of 2
	template<typename T>
	static T nextPowerOf2(T val)
	{
		--val;
		val = (val >> 1) | val;
		val = (val >> 2) | val;
		val = (val >> 4) | val;
		val = (val >> 8) | val;
		val = (val >> 16) | val;
		return val+1;
	}

	// Advance a to b by up to inc
	template<typename T>
	static T advance(T a, T b, T inc)
	{
		if (a < b) return std::min(a+inc, b);
		else return std::max(a-inc, b);
	}

	// Filter a vector
	template<typename T, typename U>
	std::vector<T> filter(std::vector<T>& in, U predicate) {
		std::vector<T> result;
		std::for_each(in.begin(), in.end(), [&](T& e) {
			if (predicate(e)) {
				result.push_back(e);
			}
		});
		return result;
	}

	// Prefetch data from memory
	static inline void prefetchL1(void* p) {
#ifdef _MSC_VER
		_mm_prefetch(static_cast<const char*>(p), _MM_HINT_T0);
#else
		__builtin_prefetch(p);
#endif
	}
	static inline void prefetchL2(void* p) {
#ifdef _MSC_VER
		_mm_prefetch(static_cast<const char*>(p), _MM_HINT_T1);
#else
		__builtin_prefetch(p);
#endif
	}
}
