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

namespace Halley {
	class Stopwatch {
	public:
		Stopwatch(bool start = true);

		void start();
		void pause();
		void reset();

		Time elapsedSeconds() const;
		long long elapsedMicroSeconds() const;
		long long elapsedNanoSeconds() const;

	private:
		std::chrono::time_point<std::chrono::system_clock> startTime;
		long long measuredTime = 0;
		bool running = false;
	};

	class StopwatchAveraging
	{
	public:
		explicit StopwatchAveraging(int nSamples = 30);
		void beginSample();
		void endSample();

		long long averageElapsedNanoSeconds() const;
		long long lastElapsedNanoSeconds() const;

	private:
		int nSamples;
		int nsTakenAvgSamples = 0;

		std::chrono::time_point<std::chrono::system_clock> startTime;

		long long nsTaken = 0;
		long long nsTakenAvg = 0;
		long long nsTakenAvgAccum = 0;
	};
}