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

#include "halley/utils/utils.h"

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
	toAdd = 0;
	startTime = high_resolution_clock::now();
	paused = false;
}

void StopwatchAveraging::endSample()
{
	auto now = high_resolution_clock::now();
	nsTaken = (paused ? 0 : duration_cast<nanoseconds>(now - startTime).count()) + toAdd;

	nsTakenAvgAccum += nsTaken;
	nsTakenAvgSamples++;
	if (nsTakenAvgSamples >= nSamples) {
		nsTakenAvg = int(nsTakenAvgAccum / nsTakenAvgSamples);
		nsTakenAvgSamples = 0;
		nsTakenAvgAccum = 0;
	}
	paused = false;
}

void StopwatchAveraging::pause()
{
	if (!paused) {
		auto now = high_resolution_clock::now();
		toAdd += duration_cast<nanoseconds>(now - startTime).count();
		paused = true;
	}
}

void StopwatchAveraging::resume()
{
	if (paused) {
		startTime = high_resolution_clock::now();
		paused = false;
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

StopwatchRollingAveraging::StopwatchRollingAveraging(size_t nSamples)
	: nsTaken(nSamples)
{}

void StopwatchRollingAveraging::setNumSamples(size_t n)
{
	nsTaken.clear();
	nsTaken.resize(n);
	pos = 0;
	samplesTaken = 0;
}

void StopwatchRollingAveraging::beginSample()
{
	toAdd = 0;
	startTime = high_resolution_clock::now();
	paused = false;
}

void StopwatchRollingAveraging::endSample()
{
	const auto now = high_resolution_clock::now();
	nsTaken[pos] = (paused ? 0 : duration_cast<nanoseconds>(now - startTime).count()) + toAdd;
	pos = (pos + 1) % nsTaken.size();
	samplesTaken++;
	paused = false;
}

void StopwatchRollingAveraging::pause()
{
	if (!paused) {
		const auto now = high_resolution_clock::now();
		toAdd += duration_cast<nanoseconds>(now - startTime).count();
		paused = true;
	}
}

void StopwatchRollingAveraging::resume()
{
	if (paused) {
		startTime = high_resolution_clock::now();
		paused = false;
	}
}

int64_t StopwatchRollingAveraging::elapsedNanoSeconds(Mode mode) const
{
	switch (mode) {
	case Mode::Average:
		return averageElapsedNanoSeconds();
	case Mode::Latest:
		return lastElapsedNanoSeconds();
	case Mode::Min:
		return minElapsedNanoSeconds();
	case Mode::Max:
		return maxElapsedNanoSeconds();
	}
	return 0;
}

int64_t StopwatchRollingAveraging::minElapsedNanoSeconds() const
{
	int64_t min = std::numeric_limits<int64_t>::max();
	const size_t n = std::min(nsTaken.size(), samplesTaken);
	for (size_t i = 0; i < n; ++i) {
		min = std::min(min, nsTaken[i]);
	}
	return min;
}

int64_t StopwatchRollingAveraging::maxElapsedNanoSeconds() const
{
	int64_t max = std::numeric_limits<int64_t>::min();
	const size_t n = std::min(nsTaken.size(), samplesTaken);
	for (size_t i = 0; i < n; ++i) {
		max = std::max(max, nsTaken[i]);
	}
	return max;
}

int64_t StopwatchRollingAveraging::averageElapsedNanoSeconds() const
{
	int64_t accum = 0;
	const size_t n = std::min(nsTaken.size(), samplesTaken);
	if (n == 0) {
		return 0;
	}
	for (size_t i = 0; i < n; ++i) {
		accum += nsTaken[i];
	}
	return accum / n;
}

int64_t StopwatchRollingAveraging::lastElapsedNanoSeconds() const
{
	if (nsTaken.empty()) {
		return 0;
	}
	const auto index = (pos + nsTaken.size() - 1) % nsTaken.size();
	return nsTaken[index];
}
