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
		ExecutionQueue();
		void addToQueue(TaskBase task);
		TaskBase getNext();
		std::vector<TaskBase> getAll();

		size_t threadCount() const;
		void onAttached();
		void onDetached();

		static ExecutionQueue& getDefault();

	private:
		std::deque<TaskBase> queue;
		boost::mutex mutex;
		boost::condition_variable condition;

		std::atomic<int> attachedCount;
		std::atomic<bool> hasTasks;
	};

	class Executors
	{
	public:
		static Executors& get();
		static void set(Executors& e);

		static ExecutionQueue& getCPU() { return instance->cpu; }
		static ExecutionQueue& getVideoAux() { return instance->videoAux; }
		static ExecutionQueue& getMainThread() { return instance->mainThread; }
		static ExecutionQueue& getDiskIO() { return instance->diskIO; }

	private:
		static Executors* instance;

		ExecutionQueue cpu;
		ExecutionQueue videoAux;
		ExecutionQueue mainThread;
		ExecutionQueue diskIO;
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
