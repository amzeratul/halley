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
#include <halley/support/exception.h>
#include <gsl/span>
#include <cstdint>

#include "range.h"
#include "halley/concurrency/mutex.h"

namespace Halley {
	class MT199937AR;

	class Random {
	public:
		static Random& getGlobal();
		static Random& getSharedGlobal();
		static Random& getThreadGlobal();

		Random(bool threadSafe = false);
		Random(uint32_t seed, bool threadSafe = false);
		Random(uint64_t seed, bool threadSafe = false);
		Random(gsl::span<const std::byte> data, bool threadSafe = false);
		~Random();

		Random(const Random& other);
		Random(Random&& other) noexcept;
		Random& operator=(const Random& other);
		Random& operator=(Random&& other) noexcept;

		// NOTE THAT THIS IS INCLUSIVE ON MAX FOR INTEGER TYPES ONLY
		int32_t getInt(Range<int32_t> range); // [min, max]
		int8_t getInt(int8_t min, int8_t max); // [min, max]
		uint8_t getInt(uint8_t min, uint8_t max); // [min, max]
		int16_t getInt(int16_t min, int16_t max); // [min, max]
		uint16_t getInt(uint16_t min, uint16_t max); // [min, max]
		int32_t getInt(int32_t min, int32_t max); // [min, max]
		uint32_t getInt(uint32_t min, uint32_t max); // [min, max]
		int64_t getInt(int64_t min, int64_t max); // [min, max]
		uint64_t getInt(uint64_t min, uint64_t max); // [min, max]
		size_t getSizeT(size_t min, size_t max); // [min, max]
		float getFloat(float min, float max); // [min, max)
		float getFloat(Range<float> range); // [range.start, range.end)
		double getDouble(double min, double max); // [min, max)

		// NOTE THAT THIS IS ALWAYS EXCLUSIVE ON MAX
		int32_t get(int32_t min, int32_t max); // [min, max)
		float get(float min, float max); // [min, max)

		template <typename T>
		size_t getRandomIndex(const T& vec)
		{
			size_t size = std::end(vec) - std::begin(vec);
			if (size == 1) {
				// Perf: common case with particles; avoids calling the RNG
				return 0;
			} else if (size == 0) {
				throw Exception("Can't get random index of empty sequence.", HalleyExceptions::Utils);
			}
			return getSizeT(size_t(0), size_t(size - 1));
		}

		template <typename T>
		auto getRandomElement(const T& vec) -> decltype(vec[0])&
		{
			return vec[getRandomIndex(vec)];
		}
		
		template <typename T>
		auto getRandomElement(T& vec) -> decltype(vec[0])&
		{
			return vec[getRandomIndex(vec)];
		}

		void getBytes(gsl::span<std::byte> dst);
		void getBytes(gsl::span<Byte> dst);
		void setSeed(uint32_t seed);
		void setSeed(uint64_t seed);
		void setSeed(gsl::span<const std::byte> data);
		void setSeed(gsl::span<Byte> data);

		uint32_t getRawInt();
		uint64_t getRawInt64();
		float getRawFloat();
		double getRawDouble();

	private:
		std::unique_ptr<MT199937AR> generator;
		std::unique_ptr<Mutex> mutex;

		uint32_t getRawIntUnsafe();

		template <typename F>
		auto withLock(F f) const
		{
			if (mutex) {
				UniqueLock lock(*mutex);
				return f();
			}
			return f();
		}
	};

}
