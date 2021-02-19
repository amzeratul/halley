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

#include "halley/maths/random.h"
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <random>
#include "mt199937ar.h"
using namespace Halley;

Random::Random()
	: generator(std::make_unique<MT199937AR>())
{
}

Random::Random(uint32_t seed)
	: generator(std::make_unique<MT199937AR>())
{
	setSeed(seed);
}

Random::Random(uint64_t seed)
	: generator(std::make_unique<MT199937AR>())
{
	setSeed(seed);
}

Random::Random(gsl::span<const gsl::byte> data)
	: generator(std::make_unique<MT199937AR>())
{
	setSeed(data);
}

Random::~Random() = default;

Random::Random(Random&& other) noexcept = default;

Random& Random::operator=(Random&& other) noexcept = default;

int32_t Random::getInt(int32_t min, int32_t max)
{
	if (min > max) {
		std::swap(min, max);
	}
	const uint32_t base = getRawInt();
	const uint32_t range = uint32_t(max - min + 1);
	if (range == 0) { // If min and max correspond to the whole range represented, this blows up
		return int32_t(base);
	}
	return int32_t(base % range) + min;
}

uint32_t Random::getInt(uint32_t min, uint32_t max)
{
	if (min > max) {
		std::swap(min, max);
	}
	const uint32_t base = getRawInt();
	const uint32_t range = max - min + 1;
	if (range == 0) { // If min and max correspond to the whole range represented, this blows up
		return base;
	}
	return base % range + min;
}

int64_t Random::getInt(int64_t min, int64_t max)
{
	if (min > max) {
		std::swap(min, max);
	}
	const int64_t base = int64_t((uint64_t(getRawInt()) << 32ull) | uint64_t(getRawInt()));
	const uint64_t range = uint64_t(max - min + 1);
	if (range == 0) { // If min and max correspond to the whole range represented, this blows up
		return int64_t(base);
	}
	return int64_t(base % range) + min;
}

uint64_t Random::getInt(uint64_t min, uint64_t max)
{
	if (min > max) {
		std::swap(min, max);
	}
	const uint64_t base = (uint64_t(getRawInt()) << 32ull) | uint64_t(getRawInt());
	const uint64_t range = max - min + 1;
	if (range == 0) { // If min and max correspond to the whole range represented, this blows up
		return base;
	}
	return base % range + min;
}

size_t Random::getSizeT(size_t min, size_t max)
{
	return size_t(getInt(uint64_t(min), uint64_t(max)));
}

float Random::getFloat(float min, float max)
{
	return getRawFloat() * (max - min) + min;
}

double Random::getDouble(double min, double max)
{
	return getRawDouble() * (max - min) + min;
}

Random& Random::getGlobal()
{
	static Random* global = nullptr;
	if (!global) {
		std::random_device rd;
		
		const time_t curTime = time(nullptr);
		const unsigned int curClock = static_cast<unsigned int>(clock());
		const unsigned int salt = 0x3F29AB51;
		unsigned int seed[] = { rd(), rd(), rd(), rd(), rd(), curClock, salt, static_cast<unsigned int>(curTime & 0xFFFFFFFF) };
		global = new Random(gsl::as_bytes(gsl::span<unsigned int>(seed)));
	}
	return *global;
}

void Random::getBytes(gsl::span<gsl::byte> dst)
{
	int step = 0;
	uint32_t number = getRawInt();

	for (int pos = 0; pos < dst.size_bytes(); ++pos) {
		dst[pos] = gsl::byte(uint8_t(number & 0xFF));

		number >>= 8;
		++step;
		if (step == 4) {
			number = getRawInt();
			step = 0;
		}
	}
}

void Random::getBytes(gsl::span<Byte> dst)
{
	getBytes(gsl::span<gsl::byte>(reinterpret_cast<gsl::byte*>(dst.data()), dst.size()));
}

void Random::setSeed(uint32_t seed)
{
	 generator->init_genrand(seed);
}

void Random::setSeed(uint64_t seed)
{
	setSeed(gsl::as_bytes(gsl::span<uint64_t>(&seed, 1)));
}

void Random::setSeed(gsl::span<const gsl::byte> data)
{
	std::vector<uint32_t> initData(alignUp(size_t(data.size_bytes()), sizeof(uint32_t)) / sizeof(uint32_t), 0);
	memcpy(initData.data(), data.data(), data.size_bytes());
	generator->init_by_array(initData.data(), initData.size());
}

void Random::setSeed(gsl::span<Byte> data)
{
	setSeed(gsl::span<gsl::byte>(reinterpret_cast<gsl::byte*>(data.data()), data.size()));
}

uint32_t Random::getRawInt()
{
	return generator->genrand_int32();
}

float Random::getRawFloat()
{
	return float(generator->genrand_real2());
}

double Random::getRawDouble()
{
	return generator->genrand_res53();
}

