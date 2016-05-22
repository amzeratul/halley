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

#pragma once

#include <queue>
#include "concurrent.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	typedef long long PriorityType;

	class Job {
	public:
		Job();
		Job(std::function<void()> f, int priority=0);
		void run();
		PriorityType getPriority() const;

		bool operator<(const Job& j) const { return priority < j.priority; }

	private:
		std::function<void()> f;
		PriorityType priority;
	};

	class JobExecuter {
	public:
		JobExecuter();

		void add(Job j);
		void add(std::function<void()> f, int priority=0);

		Maybe<Job> getNext();
		Maybe<Job> waitNext();
		void runNext();
		bool tryRunNext();

		void stop();

	private:
		bool running;
		mutex m;
		condition c;
		std::priority_queue<Job> jobs;
	};

	class JobTerminatedException : public std::exception {
	};
}
