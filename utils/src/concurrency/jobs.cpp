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

#include "jobs.h"
#include "../support/exception.h"
#include <ctime>

using namespace Halley;


Halley::Job::Job(std::function<void()> _f, int _priority)
	: f(_f)
{
	// Sort first by priority, then by negative timestamp (so older jobs have higher priority)
	unsigned int c = (unsigned int)(-clock());
	priority = (((long long)_priority) << 32) | c;
}

void Halley::Job::run()
{
	f();
}

PriorityType Halley::Job::getPriority() const
{
	return priority;
}

Halley::JobExecuter::JobExecuter()
	: running(true)
{
}

void Halley::JobExecuter::add(Job job)
{
	{
		mutex_locker lock(m);
		jobs.push(job);
	}
	c.notify_one();
}

void Halley::JobExecuter::add(std::function<void()> f, int priority/*=0*/)
{
	add(Job(f, priority));
}

boost::optional<Job> Halley::JobExecuter::getNext()
{
	mutex_locker lock(m);
	if (!running || jobs.empty()) return boost::optional<Job>();
	boost::optional<Job> result = boost::optional<Job>(jobs.top());
	jobs.pop();
	return result;
}

boost::optional<Job> Halley::JobExecuter::waitNext()
{
	mutex_locker lock(m);
	while (running && jobs.empty()) {
		c.wait(lock);
	}
	if (!running) {
		return boost::optional<Job>();
	}
	auto j = boost::optional<Job>(jobs.top());
	jobs.pop();
	return j;
}

void Halley::JobExecuter::runNext()
{
	auto job = waitNext();
	if (job) {
		job->run();
	}
}

bool Halley::JobExecuter::tryRunNext()
{
	boost::optional<Job> job = getNext();
	if (job) {
		job->run();
		return true;
	}
	return false;
}

void Halley::JobExecuter::stop()
{
	if (running) {
		{
			mutex_locker lock(m);
			running = false;
			while (!jobs.empty()) jobs.pop();
		}
		c.notify_all();
	}
}
