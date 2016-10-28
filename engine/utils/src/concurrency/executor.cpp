#include <halley/concurrency/concurrent.h>
#include <halley/concurrency/executor.h>
#include <halley/support/exception.h>
#include "halley/text/string_converter.h"

using namespace Halley;

Executors* Executors::instance = nullptr;

ExecutionQueue::ExecutionQueue()
	: aborted(false)
{
	hasTasks.store(false);
}

TaskBase ExecutionQueue::getNext()
{
	std::unique_lock<std::mutex> lock(mutex);
	while (queue.empty()) {
		if (!aborted) {
			condition.wait(lock);
		}
		if (aborted) {
			queue.clear();
			return TaskBase([] () {});
		}
	}

	TaskBase value = queue.front();
	queue.pop_front();
	return value;
}

std::vector<TaskBase> ExecutionQueue::getAll()
{
	std::unique_lock<std::mutex> lock(mutex);
	hasTasks.store(false);
	std::vector<TaskBase> tasks(queue.begin(), queue.end());
	queue.clear();
	return tasks;
}

void ExecutionQueue::addToQueue(TaskBase task)
{
#if HAS_THREADS
	std::unique_lock<std::mutex> lock(mutex);
	queue.emplace_back(task);
	hasTasks.store(true);

	condition.notify_one();
#else
	task();
#endif
}

Executors& Executors::get()
{
	if (!instance) {
		throw Exception("Executors instance not defined");
	}
	return *instance;
}

void Executors::set(Executors& e)
{
	instance = &e;
}

size_t ExecutionQueue::threadCount() const
{
	return attachedCount.load();
}

void ExecutionQueue::onAttached()
{
	++attachedCount;
}

void ExecutionQueue::onDetached()
{
	--attachedCount;
}

void ExecutionQueue::abort()
{
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (aborted) {
			return;
		}
		aborted = true;
	}
	condition.notify_all();
}

ExecutionQueue& ExecutionQueue::getDefault()
{
	return Executors::get().getCPU();
}

Executor::Executor(ExecutionQueue& queue)
	: queue(queue)
	, running(true)
{
#if HAS_THREADS
	queue.onAttached();
#endif
}

Executor::~Executor()
{
#if HAS_THREADS
	queue.onDetached();
#endif
}

bool Executor::runPending()
{
#if HAS_THREADS
	auto tasks = queue.getAll();
	for (auto& t : tasks) {
		t();
	}
#endif
	return false;
}

void Executor::runForever()
{
#if HAS_THREADS
	while (running)	{
		auto next = queue.getNext();
		if (running) {
			next();
		}
	}
#endif
}

void Executor::stop()
{
#if HAS_THREADS
	running = false;
	queue.abort();
#endif
}

ThreadPool::ThreadPool(ExecutionQueue& queue, size_t n)
{
#if HAS_THREADS
	for (size_t i = 0; i < n; i++) {
		executors.emplace_back(std::make_unique<Executor>(queue));
	}
	threads.resize(n);

	for (size_t i = 0; i < n; i++) {
		threads[i] = std::thread([this, i]()
		{
			Concurrent::setThreadName("threadPool" + toString(i));
			executors[i]->runForever();
		});
	}
#endif
}

ThreadPool::~ThreadPool()
{
#if HAS_THREADS
	for (auto& e: executors) {
		e->stop();
	}
	for (auto& t : threads) {
		t.join();
	}
#endif
}
