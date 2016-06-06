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

#include <random>
#include <halley/utils/utils.h>

namespace Halley {
	typedef std::mt19937 RandomGenType;

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
			if (min > max) {
				std::swap(min, max);
			}
			std::uniform_int_distribution<T> dist(min, max);
			return dist(generator);
		}

		template <typename T>
		T getFloat(T min, T max)
		{
			if (min > max) std::swap(min, max);
			std::uniform_real_distribution<T> dist(min, max);
			return dist(generator);
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