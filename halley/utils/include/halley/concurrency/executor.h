#pragma once
#include <deque>
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

		Executor();
		void addToQueue(TaskBase task);
		TaskBase getNext();

		size_t threadCount() const;
		void onAttached();
		void onDetached();

	private:
		std::deque<TaskBase> queue;
		boost::mutex mutex;
		boost::condition_variable condition;

		std::atomic<int> attachedCount;
		std::atomic<bool> hasTasks;

		static Executor* defaultExecutor;
	};

	class ExecutorRunner
	{
	public:
		ExecutorRunner(Executor& queue);
		~ExecutorRunner();
		bool runPending();
		void runForever();
		void stop();
		
		static void makeThreadPool(Executor& queue, size_t n);

	private:
		Executor& queue;
		bool running = false;
	};
}
