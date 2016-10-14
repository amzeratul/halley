#pragma once

#include <atomic>
#include <thread>
#include <condition_variable>

namespace Halley
{
	class Resource
	{
	public:
		virtual ~Resource() {}
	};

	class AsyncResource : public Resource
	{
	public:
		AsyncResource();
		virtual ~AsyncResource();

		void startLoading(); // call from main thread before spinning worker thread
		void doneLoading();  // call from worker thread when done loading
		void waitForLoad() const;

		bool isLoaded() const;

	private:
		std::atomic<bool> loading;
		mutable std::condition_variable loadWait;
		mutable std::mutex loadMutex;
	};
}
