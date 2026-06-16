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
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"
using namespace Halley;

Random::Random(bool threadSafe)
	: generator(std::make_unique<MT199937AR>())
{
	if (threadSafe) {
		mutex = std::make_unique<Mutex>();
	}
}

Random::Random(uint32_t seed, bool threadSafe)
	: generator(std::make_unique<MT199937AR>())
{
	if (threadSafe) {
		mutex = std::make_unique<Mutex>();
	}
	setSeed(seed);
}

Random::Random(uint64_t seed, bool threadSafe)
	: generator(std::make_unique<MT199937AR>())
{
	if (threadSafe) {
		mutex = std::make_unique<Mutex>();
	}
	setSeed(seed);
}

Random::Random(gsl::span<const std::byte> data, bool threadSafe)
	: generator(std::make_unique<MT199937AR>())
{
	if (threadSafe) {
		mutex = std::make_unique<Mutex>();
	}
	setSeed(data);
}

Random::~Random()
{
}

Random::Random(const Random& other)
{
	*this = other;
}

Random::Random(Random&& other) noexcept
{
	*this = std::move(other);
}

Random& Random::operator=(const Random& other)
{
	if (this == &other) {
		return *this;
	}

	other.withLock([&] {
		if (other.mutex) {
			mutex = std::make_unique<Mutex>();
		}
		generator = std::make_unique<MT199937AR>(*other.generator);
	});
	return *this;
}

Random& Random::operator=(Random&& other) noexcept
{
	other.withLock([&] {
		if (other.mutex) {
			mutex = std::make_unique<Mutex>();
		}
		generator = std::move(other.generator);
	});
	return *this;
}

int32_t Random::getInt(Range<int32_t> range)
{
	return getInt(range.start, range.end);
}

int8_t Random::getInt(int8_t min, int8_t max)
{
	if (min > max) {
		std::swap(min, max);
	}
	const uint8_t base = getRawInt();
	if (min == std::numeric_limits<int8_t>::min() && max == std::numeric_limits<int8_t>::max()) {
		return int8_t(base);
	}
	const uint8_t range = uint8_t(max - min + 1);
	return int8_t(base % range) + min;
}

uint8_t Random::getInt(uint8_t min, uint8_t max)
{
	if (min > max) {
		std::swap(min, max);
	}
	const uint8_t base = getRawInt();
	const uint8_t range = max - min + 1;
	if (range == 0) { // If min and max correspond to the whole range represented, this blows up
		return base;
	}
	return base % range + min;
}

int16_t Random::getInt(int16_t min, int16_t max)
{
	if (min > max) {
		std::swap(min, max);
	}
	const uint16_t base = getRawInt();
	if (min == std::numeric_limits<int16_t>::min() && max == std::numeric_limits<int16_t>::max()) {
		return int16_t(base);
	}
	const uint16_t range = uint16_t(max - min + 1);
	return int16_t(base % range) + min;
}

uint16_t Random::getInt(uint16_t min, uint16_t max)
{
	if (min > max) {
		std::swap(min, max);
	}
	const uint16_t base = getRawInt();
	const uint16_t range = max - min + 1;
	if (range == 0) { // If min and max correspond to the whole range represented, this blows up
		return base;
	}
	return base % range + min;
}

int32_t Random::getInt(int32_t min, int32_t max)
{
	if (min > max) {
		std::swap(min, max);
	}
	const uint32_t base = getRawInt();
	if (min == std::numeric_limits<int32_t>::min() && max == std::numeric_limits<int32_t>::max()) {
		return int32_t(base);
	}
	const uint32_t range = uint32_t(max - min + 1);
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
	const int64_t base = int64_t(getRawInt64());
	if (min == std::numeric_limits<int64_t>::min() && max == std::numeric_limits<int64_t>::max()) {
		return int64_t(base);
	}
	const uint64_t range = uint64_t(max - min + 1);
	return int64_t(base % range) + min;
}

uint64_t Random::getInt(uint64_t min, uint64_t max)
{
	if (min > max) {
		std::swap(min, max);
	}
	const uint64_t base = getRawInt64();
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

float Random::getFloat(Range<float> range)
{
	return getFloat(range.start, range.end);
}

double Random::getDouble(double min, double max)
{
	return getRawDouble() * (max - min) + min;
}

int32_t Random::get(int32_t min, int32_t max)
{
	if (min >= max) {
		throw Exception("Unable to generate Random int32_t between " + toString(min) + " and " + toString(max), HalleyExceptions::Utils);
	}

	const auto value = getInt(min, max - 1);
	HalleyAssertDev(value >= min);
	HalleyAssertDev(value < max);
	return value;
}

float Random::get(float min, float max)
{
	if (min >= max) {
		throw Exception("Unable to generate Random float between " + toString(min) + " and " + toString(max), HalleyExceptions::Utils);
	}

	const auto value = getFloat(min, max);
	return value;
}

Random& Random::getSharedGlobal()
{
	static std::unique_ptr<Random> global = nullptr;
	if (!global) [[unlikely]] {
		std::random_device rd;
		
		const time_t curTime = time(nullptr);
		const unsigned int curClock = static_cast<unsigned int>(clock());
		const unsigned int salt = 0x3F29AB51;
		unsigned int seed[] = { rd(), rd(), rd(), rd(), rd(), curClock, salt, static_cast<unsigned int>(curTime & 0xFFFFFFFF), rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
		global = std::make_unique<Random>(gsl::as_bytes(gsl::span<unsigned int>(seed)), true);
	}
	return *global;
}

Random& Random::getThreadGlobal()
{
	thread_local static std::unique_ptr<Random> threadGlobal;
	if (!threadGlobal) [[unlikely]] {
		threadGlobal = std::make_unique<Random>(getSharedGlobal().getRawInt64());
	}
	return *threadGlobal;
}

Random& Random::getGlobal()
{
	return getThreadGlobal();
}

void Random::getBytes(gsl::span<std::byte> dst)
{
	withLock([&] {
		int step = 3;
		uint32_t number = 0;

		for (int pos = 0; pos < dst.size_bytes(); ++pos) {
			if (++step == 4) {
				number = getRawIntUnsafe();
				step = 0;
			}

			dst[pos] = static_cast<std::byte>(static_cast<uint8_t>(number & 0xFF));
			number >>= 8;
		}
	});
}

void Random::getBytes(gsl::span<Byte> dst)
{
	getBytes(gsl::span<std::byte>(reinterpret_cast<std::byte*>(dst.data()), dst.size()));
}

void Random::setSeed(uint32_t seed)
{
	 generator->init_genrand(seed);
}

void Random::setSeed(uint64_t seed)
{
	setSeed(gsl::as_bytes(gsl::span<uint64_t>(&seed, 1)));
}

void Random::setSeed(gsl::span<const std::byte> data)
{
	Vector<uint32_t> initData(alignUp(size_t(data.size_bytes()), sizeof(uint32_t)) / sizeof(uint32_t), 0);
	memcpy(initData.data(), data.data(), data.size_bytes());
	generator->init_by_array(initData.data(), initData.size());
}

void Random::setSeed(gsl::span<Byte> data)
{
	setSeed(gsl::span<std::byte>(reinterpret_cast<std::byte*>(data.data()), data.size()));
}

uint32_t Random::getRawInt()
{
	return withLock([&] {
		return generator->genrand_int32();
	});
}

uint64_t Random::getRawInt64()
{
	return withLock([&] {
		const auto a = generator->genrand_int32();
		const auto b = generator->genrand_int32();
		return (static_cast<uint64_t>(a) << 32ull) | static_cast<uint64_t>(b);
	});
}

float Random::getRawFloat()
{
	return withLock([&] {
		return static_cast<float>(generator->genrand_real2());
	});
}

double Random::getRawDouble()
{
	return withLock([&] {
		return generator->genrand_res53();
	});
}

uint32_t Random::getRawIntUnsafe()
{
	return generator->genrand_int32();
}

