#pragma once
#include <deque>
#include <boost/thread.hpp>
#include <functional>
#include <atomic>

namespace Halley
{
	using TaskBase = std::function<void()>;

	class ExecutionQueue
	{
	public:
		static ExecutionQueue& getDefault();
		static void setDefault(ExecutionQueue& e);

		ExecutionQueue();
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

		static ExecutionQueue* defaultQueue;
	};

	class Executor
	{
	public:
		Executor(ExecutionQueue& queue);
		~Executor();
		bool runPending();
		void runForever();
		void stop();
		
		static void makeThreadPool(ExecutionQueue& queue, size_t n);

	private:
		ExecutionQueue& queue;
		bool running = false;
	};
}
