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

#ifdef __ANDROID__
	#include <tr1/random>
	#define randnamespace std::tr1
	#define uniform_int_dist uniform_int
	#define uniform_real_dist uniform_real
#else
	#include <random>
	#define randnamespace std
	#define uniform_int_dist uniform_int_distribution
	#define uniform_real_dist uniform_real_distribution
#endif

#include <halley/utils/utils.h>

namespace Halley {
	typedef randnamespace::mt19937 RandomGenType;

	class Random {
	public:
		static Random& getGlobal();

		Random();
		Random(long seed);
		Random(char* bytes, size_t nBytes);

		template <typename T> T get(T max) { return get(T(0), max); }
		template <typename T> T get(T min, T max);

		template <typename T>
		T getInt(T min, T max)
		{
			if (min > max) swap(min, max);
			randnamespace::uniform_int_dist<T> dist(min, max);
			return dist(generator);
		}

		template <typename T>
		T getFloat(T min, T max)
		{
			if (min > max) std::swap(min, max);
#ifdef __ANDROID__
			// Hack
			int range = 1000000;
			int raw = getInt(0, range);
			return min + raw * ((max-min)/1000000);
#else
			randnamespace::uniform_real_dist<T> dist(min, max);
			return dist(generator);
#endif
		}

		void setSeed(long seed);
		void setSeed(char* bytes, size_t nBytes);

	private:
		RandomGenType generator;
	};


	template <typename T>
	struct _getHack {
		static T get(Random* rng, T min, T max) { return rng->getInt(min, max); }
	};

	template <>
	struct _getHack<float> {
		static float get(Random *rng, float min, float max) { return rng->getFloat(min, max); }
	};

	template <>
	struct _getHack<double> {
		static double get(Random *rng, double min, double max) { return rng->getFloat(min, max); }
	};

	template <typename T>
	T Random::get(T min, T max)
	{
		return _getHack<T>::get(this, min, max);
	}

}