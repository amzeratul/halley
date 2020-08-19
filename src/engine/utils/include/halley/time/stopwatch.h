#pragma once

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

#include "halleytime.h"
#include <chrono>
#include <cstdint>

namespace Halley {
	class Stopwatch {
	public:
		Stopwatch(bool start = true);

		void start();
		void pause();
		void reset();

		Time elapsedSeconds() const;
		int64_t elapsedMilliseconds() const;
		int64_t elapsedMicroseconds() const;
		int64_t elapsedNanoseconds() const;

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
		int64_t measuredTime = 0;
		bool running = false;
	};

	class StopwatchAveraging
	{
	public:
		enum class Mode
		{
			Average,
			Latest
		};

		explicit StopwatchAveraging(int nSamples = 30);
		void beginSample();
		void endSample();
		void pause();
		void resume();

		int64_t elapsedNanoSeconds(Mode mode) const;
		int64_t averageElapsedNanoSeconds() const;
		int64_t lastElapsedNanoSeconds() const;

	private:
		int nSamples;
		int nsTakenAvgSamples = 0;
		bool paused = false;

		std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
		int64_t toAdd = 0;

		int64_t nsTaken = 0;
		int64_t nsTakenAvg = 0;
		int64_t nsTakenAvgAccum = 0;
	};

	class StopwatchRollingAveraging
	{
	public:
		enum class Mode
		{
			Average,
			Latest,
			Min,
			Max
		};

		explicit StopwatchRollingAveraging(size_t nSamples = 30);

		void setNumSamples(size_t n);

		void beginSample();
		void endSample();
		void pause();
		void resume();

		int64_t elapsedNanoSeconds(Mode mode) const;
		int64_t minElapsedNanoSeconds() const;
		int64_t maxElapsedNanoSeconds() const;
		int64_t averageElapsedNanoSeconds() const;
		int64_t lastElapsedNanoSeconds() const;

	private:
		std::vector<int64_t> nsTaken;
		size_t pos = 0;
		size_t samplesTaken = 0;
		bool paused = false;

		std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
		int64_t toAdd = 0;
	};
}