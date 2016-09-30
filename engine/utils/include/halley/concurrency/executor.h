#pragma once
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <vector>

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
		void abort();

		static ExecutionQueue& getDefault();

	private:
		std::deque<TaskBase> queue;
		std::mutex mutex;
		std::condition_variable condition;

		std::atomic<int> attachedCount;
		std::atomic<bool> hasTasks;
		std::atomic<bool> aborted;
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
		
	private:
		ExecutionQueue& queue;
		std::atomic<bool> running;
	};

	class ThreadPool
	{
	public:
		ThreadPool(ExecutionQueue& queue, size_t n);
		~ThreadPool();

	private:
		std::vector<std::unique_ptr<Executor>> executors;
		std::vector<std::thread> threads;
	};
}
