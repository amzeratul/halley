#pragma once
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include "halley/data_structures/vector.h"
#include "halley/text/halleystring.h"

namespace Halley
{
	using TaskBase = std::function<void()>;

	class ExecutionQueue
	{
	public:
		ExecutionQueue();
		void addToQueue(TaskBase task);

		TaskBase getNext();
		Vector<TaskBase> getUpTo(size_t n);
		Vector<TaskBase> getAll();

		size_t threadCount() const;
		void onAttached();
		void onDetached();
		void abort();

		void setImmediate(bool immediate);

		static ExecutionQueue& getDefault();

	private:
		std::deque<TaskBase> queue;
		std::mutex mutex;
		std::condition_variable condition;

		std::atomic<int> attachedCount;
		std::atomic<bool> hasTasks;
		std::atomic<bool> aborted;

		bool immediate = false;
	};

	class Executors
	{
	public:
		Executors();

		static Executors& get();
		static void setInstance(Executors& e);

		static ExecutionQueue& getCPU() { return instance->cpu; }
		static ExecutionQueue& getCPUAux() { return instance->cpuAux; }
		static ExecutionQueue& getVideoAux() { return instance->videoAux; }
		static ExecutionQueue& getMainUpdateThread() { return instance->mainUpdateThread; }
		static ExecutionQueue& getMainRenderThread() { return instance->mainRenderThread; }
		static ExecutionQueue& getDiskIO() { return instance->diskIO; }
		static ExecutionQueue& getImmediate() { return instance->immediate; }

		[[deprecated]] static ExecutionQueue& getMainThread() { return instance->mainUpdateThread; }

	private:
		static Executors* instance;

		ExecutionQueue cpu;
		ExecutionQueue cpuAux;
		ExecutionQueue videoAux;
		ExecutionQueue mainUpdateThread;
		ExecutionQueue mainRenderThread;
		ExecutionQueue diskIO;
		ExecutionQueue immediate;
	};

	class Executor
	{
	public:
		Executor(ExecutionQueue& queue);
		~Executor();

		void runUpTo(size_t n);
		void runPending();
		void runForever();
		void stop();
		
	private:
		ExecutionQueue& queue;
		std::atomic<bool> running;
	};

	class SingleThreadExecutor {
	public:
		using MakeThread = std::function<std::thread(String, std::function<void()>)>;

		SingleThreadExecutor(String name, MakeThread makeThread);
		~SingleThreadExecutor();

		ExecutionQueue& getQueue();

		void stop();

	private:
		ExecutionQueue queue;
		Executor executor;
		std::thread thread;
	};

	class ThreadPool
	{
	public:
		using MakeThread = std::function<std::thread(String, std::function<void()>)>;

		ThreadPool(const String& name, ExecutionQueue& queue, size_t n, MakeThread makeThread);
		~ThreadPool();

	private:
		String name;
		Vector<std::unique_ptr<Executor>> executors;
		Vector<std::thread> threads;
	};
}
