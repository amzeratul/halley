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

  Copyright (c) 2007-2014 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "stopwatch.h"

using namespace Halley;

/*
Stopwatch::Stopwatch(bool _start)
{
	if (_start) {
		start();
	}
}

Time Stopwatch::elapsed()
{
	long long t = extraTime;
	if (running) {
		t += SDL_GetPerformanceCounter() - startTime;
	}
	return Time(t) / Time(SDL_GetPerformanceFrequency());
}

int Halley::Stopwatch::elapsedUs()
{
	return int(elapsed() * 1000000);
}

void Halley::Stopwatch::start()
{
	if (!running) {
		running = true;
		startTime = SDL_GetPerformanceCounter();
	}
}

void Halley::Stopwatch::stop()
{
	if (running) {
		running = false;
		extraTime += SDL_GetPerformanceCounter() - startTime;
	}
}

void Halley::Stopwatch::reset()
{
	startTime = SDL_GetPerformanceCounter();
	extraTime = 0;
}
*/
