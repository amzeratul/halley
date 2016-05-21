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

#include "../../include/halley/time/stopwatch.h"

using namespace Halley;
using namespace std::chrono;

Stopwatch::Stopwatch(bool _start)
{
	if (_start) {
		start();
	}
}

void Stopwatch::start()
{
	auto now = high_resolution_clock::now();
	if (!running) {
		running = true;
		startTime = now;
	}
}

void Stopwatch::pause()
{
	auto now = high_resolution_clock::now();
	if (running) {
		running = false;
		measuredTime += duration_cast<nanoseconds>(now - startTime).count();
	}
}

void Stopwatch::reset()
{
	measuredTime = 0;
}

Time Stopwatch::elapsedSeconds() const
{
	return measuredTime / 1'000'000'000.0;
}

long long Stopwatch::elapsedMicroSeconds() const
{
	return (measuredTime + 500) / 1000;
}

long long Stopwatch::elapsedNanoSeconds() const
{
	return measuredTime;
}

StopwatchAveraging::StopwatchAveraging(int nSamples)
	: nSamples(nSamples)
{	
}

void StopwatchAveraging::beginSample()
{
	startTime = high_resolution_clock::now();
}

void StopwatchAveraging::endSample()
{
	auto now = high_resolution_clock::now();
	nsTaken = duration_cast<nanoseconds>(now - startTime).count();

	nsTakenAvgAccum += nsTaken;
	nsTakenAvgSamples++;
	if (nsTakenAvgSamples >= 30) {
		nsTakenAvg = int(nsTakenAvgAccum / nsTakenAvgSamples);
		nsTakenAvgSamples = 0;
		nsTakenAvgAccum = 0;
	}
}

long long StopwatchAveraging::averageElapsedNanoSeconds() const
{
	return nsTakenAvg;
}

long long StopwatchAveraging::lastElapsedNanoSeconds() const
{
	return nsTaken;
}
