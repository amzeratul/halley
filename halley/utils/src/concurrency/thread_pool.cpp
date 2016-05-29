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

#include <iostream>
#include "halley/concurrency/thread_pool.h"
#include <thread>
using namespace Halley;

Halley::ThreadPool::ThreadPool(int n)
{
	if (n == -1) n = std::thread::hardware_concurrency() * 3 / 2;
	n = std::max(2, n);

	running = true;

	for (int i=0; i<n; i++) {
		threads.push_back(thread([=] () {
			Concurrent::setThreadName("pool" + String::integerToString(i));
			while (running) {
				jobs.runNext();
			}
		}));
	}
}

Halley::ThreadPool::~ThreadPool()
{
	stop();
}

void Halley::ThreadPool::add(std::function<void()> f, int priority)
{
	jobs.add(f, priority);
}

void Halley::ThreadPool::stop()
{
	if (running) {
		running = false;

		jobs.stop();
		size_t sz = threads.size();
		for (size_t i=0; i<sz; i++) {
			threads[i].join();
		}
	}
}

ThreadPool& Halley::ThreadPool::get()
{
	if (!instance) instance = std::shared_ptr<ThreadPool>(new ThreadPool());
	return *instance;
}

void Halley::ThreadPool::run(std::function<void()> f, int priority/*=0*/)
{
	get().add(f, priority);
}

std::shared_ptr<ThreadPool> Halley::ThreadPool::instance;
