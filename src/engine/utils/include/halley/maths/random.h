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

#include <halley/utils/utils.h>
#include <gsl/span>
#include <cstdint>

namespace Halley {
	class MT199937AR;

	class Random {
	public:
		static Random& getGlobal();

		Random();
		Random(uint32_t seed);
		Random(gsl::span<const gsl::byte> data);
		~Random();

		int32_t getInt(int32_t min, int32_t max); // [min, max]
		uint32_t getInt(uint32_t min, uint32_t max); // [min, max]
		int64_t getInt(int64_t min, int64_t max); // [min, max]
		uint64_t getInt(uint64_t min, uint64_t max); // [min, max]
		float getFloat(float min, float max); // [min, max)
		double getDouble(double min, double max); // [min, max)

		template <typename T>
		size_t getRandomIndex(const T& vec)
		{
			return getInt(size_t(0), size_t(vec.size() - 1));
		}

		template <typename T>
		auto getRandomElement(T& vec) -> decltype(vec[0])&
		{
			return vec[getRandomIndex(vec)];
		}

		void getBytes(gsl::span<gsl::byte> dst);
		void setSeed(uint32_t seed);
		void setSeed(gsl::span<const gsl::byte> data);

		uint32_t getRawInt();
		float getRawFloat();
		double getRawDouble();

	private:
		std::unique_ptr<MT199937AR> generator;
	};

}
