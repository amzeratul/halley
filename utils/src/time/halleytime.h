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
//#include "fixed_point.h"

namespace Halley {
	/*
	template <typename T>
	class HalleyTime {
	public:
		HalleyTime() : value() {}
		explicit HalleyTime(T v) : value(v) {}

		HalleyTime<T> operator+(HalleyTime<T> p) const { return HalleyTime<T>(value+p.value); }
		HalleyTime<T> operator-(HalleyTime<T> p) const { return HalleyTime<T>(value-p.value); }
		HalleyTime<T> operator*(HalleyTime<T> p) const { return HalleyTime<T>(value*p.value); }
		HalleyTime<T> operator/(HalleyTime<T> p) const { return HalleyTime<T>(value/p.value); }
		HalleyTime<T> operator%(HalleyTime<T> p) const { return HalleyTime<T>(value%p.value); }

		HalleyTime<T>& operator+=(HalleyTime<T> p) { value += p.value; return *this; }
		HalleyTime<T>& operator-=(HalleyTime<T> p) { value -= p.value; return *this; }
		HalleyTime<T>& operator*=(HalleyTime<T> p) { value *= p.value; return *this; }
		HalleyTime<T>& operator/=(HalleyTime<T> p) { value /= p.value; return *this; }
		HalleyTime<T>& operator%=(HalleyTime<T> p) { value %= p.value; return *this; }

		HalleyTime<T>& operator=(HalleyTime<T> p) { value = p.value; return *this; }

		bool operator==(HalleyTime<T> p) const { return value == p.value; }
		bool operator!=(HalleyTime<T> p) const { return value != p.value; }
		bool operator<(HalleyTime<T> p) const { return value < p.value; }
		bool operator>(HalleyTime<T> p) const { return value > p.value; }
		bool operator<=(HalleyTime<T> p) const { return value <= p.value; }
		bool operator>=(HalleyTime<T> p) const { return value >= p.value; }

		//operator float() { return value; }
		T getValue() const { return value; }

	private:
		T value;
	};

	template <typename T> HalleyTime<T> operator*(float a, HalleyTime<T> b) { return HalleyTime<T>(a * (float)b.getValue()); }
	template <typename T> HalleyTime<T> operator/(float a, HalleyTime<T> b) { return HalleyTime<T>(a / (float)b.getValue()); }
	template <typename T> HalleyTime<T> operator*(HalleyTime<T> a, float b) { return HalleyTime<T>((float)a.getValue() * b); }
	template <typename T> HalleyTime<T> operator/(HalleyTime<T> a, float b) { return HalleyTime<T>((float)a.getValue() / b); }
	
	typedef HalleyTime<float> Time;
	template <typename U> U TimeTo(Time p) { return (U)p.getValue(); }
	*/

	typedef double Time;
	//typedef fpml::fixed_point<int, 21> Time;
	template <typename U> U TimeTo(Time p) { return (U)p; }
}
