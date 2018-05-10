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
#include <ctime>
#include <cstdlib>
using namespace Halley;

Random::Random()
{
}

Random::Random(long seed)
	: generator(int(seed))
{
}

Random::Random(char* bytes, size_t nBytes)
{
	setSeed(bytes, nBytes);
}

Random& Random::getGlobal()
{
	static Random* global = nullptr;
	if (!global) {
		time_t curTime = time(nullptr);
		int curClock = int(clock());
		int salt = 0x3F29AB51;
		int seed[] = { int(curTime & 0xFFFFFFFF), int(static_cast<long long>(curTime) >> 32), curClock, salt };
		global = new Random(reinterpret_cast<char*>(seed), sizeof(seed));
	}
	return *global;
}

void Random::getBytes(gsl::span<gsl::byte> dst)
{
	int step = 0;
	uint_fast32_t number = generator();

	for (int pos = 0; pos < dst.size_bytes(); ++pos) {
		dst[pos] = gsl::byte(uint_fast8_t(number & 0xFF));

		number >>= 8;
		++step;
		if (step == 4) {
			number = generator();
			step = 0;
		}
	}
}

void Random::setSeed(long seed)
{
	generator.seed(static_cast<int>(seed));
}

void Random::setSeed(char* bytes, size_t nBytes)
{
	std::seed_seq seq(bytes, bytes+nBytes);
	generator.seed(seq);
}
