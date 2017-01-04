#pragma once

#include <atomic>
#include <thread>
#include <condition_variable>
#include "metadata.h"

namespace Halley
{
	class Resource
	{
	public:
		virtual ~Resource() {}

		void setMeta(const Metadata& meta);
		const Metadata& getMeta() const;

	private:
		Metadata meta;
	};

	class AsyncResource : public Resource
	{
	public:
		AsyncResource();
		virtual ~AsyncResource();

		void startLoading(); // call from main thread before spinning worker thread
		void doneLoading();  // call from worker thread when done loading
		void loadingFailed(); // Call from worker thread if loading fails
		void waitForLoad() const;

		bool isLoaded() const;

	private:
		std::atomic<bool> failed;
		std::atomic<bool> loading;
		mutable std::condition_variable loadWait;
		mutable std::mutex loadMutex;
	};
}
