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

#include "jobs.h"
#include "../maths/utils.h"

namespace Halley {
	class ThreadPool {
	public:
		ThreadPool(int n=-1);
		~ThreadPool();
		void add(std::function<void()> f, int priority=0);
		void stop();

		static ThreadPool& get();
		static void run(std::function<void()> f, int priority=0);

	private:
		std::vector<thread> threads;
		std::atomic<bool> running;
		JobExecuter jobs;

		static std::shared_ptr<ThreadPool> instance;
	};
}
