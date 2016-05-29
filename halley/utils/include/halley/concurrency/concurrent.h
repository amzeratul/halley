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

#include <thread>
#include <mutex>
#include <future>
#include "halley/text/halleystring.h"

namespace Halley {
	using std::thread;
	using std::mutex;
	typedef std::condition_variable condition;
	typedef std::unique_lock<mutex> mutex_locker;
	using std::future;

	namespace Concurrent {
		template <typename T>
		inline thread run(T function)
		{
			return thread(function);
		}

		template <typename V, typename T>
		inline future<V> runFuture(T function)
		{
			std::packaged_task<T> pt(function);
			thread t(std::move(pt));
			return pt.get_future();
		}

		void setThreadName(String name);
		String getThreadName();
	}
}

/*
#define synchronized(M) for (Halley::MutexLocker synchronized_mutexlocker(M); !synchronized_mutexlocker.__tapped(); synchronized_mutexlocker.__tap())
*/
