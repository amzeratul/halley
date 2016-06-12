#pragma once
#include <vector>
#include <boost/thread.hpp>
#include <functional>
#include <atomic>

namespace Halley
{
	using TaskBase = std::function<void()>;

	class Executor
	{
	public:
		static Executor& getDefault();
		static void setDefault(Executor& e);
		static Executor* createDefault();

		Executor();
		void addToQueue(TaskBase task);
		bool runPending();
		void runForever();
		void stop();

		size_t threadCount() const;

	private:
		std::vector<TaskBase> queue;
		boost::mutex mutex;
		boost::condition_variable condition;
		std::atomic<bool> hasTasks;
		bool running = false;

		static Executor* defaultExecutor;
	};
}
