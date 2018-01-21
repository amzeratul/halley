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

Halley::Random::Random(long seed)
	: generator(int(seed))
{
}

Halley::Random::Random(char* bytes, size_t nBytes)
{
	setSeed(bytes, nBytes);
}

Random& Halley::Random::getGlobal()
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

void Halley::Random::setSeed(long seed)
{
	generator.seed(static_cast<int>(seed));
}

void Halley::Random::setSeed(char* bytes, size_t nBytes)
{
	std::seed_seq seq(bytes, bytes+nBytes);
	generator.seed(seq);
}
