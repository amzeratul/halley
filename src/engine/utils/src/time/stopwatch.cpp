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

#include "halley/time/stopwatch.h"

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

int64_t Stopwatch::elapsedMilliseconds() const
{
	return (measuredTime + 500000) / 1000000;
}

int64_t Stopwatch::elapsedMicroseconds() const
{
	return (measuredTime + 500) / 1000;
}

int64_t Stopwatch::elapsedNanoseconds() const
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
	if (nsTakenAvgSamples >= nSamples) {
		nsTakenAvg = int(nsTakenAvgAccum / nsTakenAvgSamples);
		nsTakenAvgSamples = 0;
		nsTakenAvgAccum = 0;
	}
}

int64_t StopwatchAveraging::elapsedNanoSeconds(Mode mode) const
{
	return mode == Mode::Average ? averageElapsedNanoSeconds() : lastElapsedNanoSeconds();
}

int64_t StopwatchAveraging::averageElapsedNanoSeconds() const
{
	return nsTakenAvg;
}

int64_t StopwatchAveraging::lastElapsedNanoSeconds() const
{
	return nsTaken;
}
