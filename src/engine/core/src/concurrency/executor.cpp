#include <halley/concurrency/concurrent.h>
#include <halley/concurrency/executor.h>
#include <halley/support/exception.h>

#include "halley/game/game_platform.h"
#include "halley/support/debug.h"
#include "halley/text/string_converter.h"
#include "halley/support/logger.h"

using namespace Halley;

Executors* Executors::instance = nullptr;

ExecutionQueue::ExecutionQueue()
	: attachedCount(0)
	, aborted(false)
{
}

ExecutionQueue::Entry ExecutionQueue::getNext()
{
	UniqueLock lock(mutex);
	while (queue.empty()) {
		if (!aborted) {
			condition.wait(lock);
		}
		if (aborted) {
			queue.clear();
			return { TaskBase([] () {}) };
		}
	}

	Entry value = queue.front();
	queue.pop_front();
	return value;
}

Vector<ExecutionQueue::Entry> ExecutionQueue::getUpTo(size_t n)
{
	UniqueLock lock(mutex);
	if (queue.size() <= n) {
		Vector<Entry> tasks(queue.begin(), queue.end());
		queue.clear();
		return tasks;
	} else {
		Vector<Entry> tasks(queue.begin(), queue.begin() + n);
		queue.erase(queue.begin(), queue.begin() + n);
		return tasks;
	}
}

Vector<ExecutionQueue::Entry> ExecutionQueue::getAll()
{
	UniqueLock lock(mutex);
	Vector<Entry> tasks(queue.begin(), queue.end());
	queue.clear();
	return tasks;
}

void ExecutionQueue::addToQueue(TaskBase task, std::string_view name)
{
	if (immediate) {
		task();
	} else {
		{
			UniqueLock lock(mutex);
			queue.emplace_back(task, name);
			// NB: Unlocking before notifying
		}

		condition.notifyOne();
	}
}

Executors::Executors()
{
	immediate.setImmediate(true);
}

Executors& Executors::get()
{
	if (!instance) {
		throw Exception("Executors instance not defined", HalleyExceptions::Concurrency);
	}
	return *instance;
}

void Executors::setInstance(Executors& e)
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
		UniqueLock lock(mutex);
		if (aborted) {
			return;
		}
		aborted = true;
	}
	condition.notifyAll();
}

void ExecutionQueue::setImmediate(bool immediate)
{
	this->immediate = immediate;
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

void Executor::runUpTo(size_t n)
{
#if HAS_THREADS
	auto tasks = queue.getUpTo(n);
	for (auto& t : tasks) {
		t.task();
	}
#endif
}

void Executor::runPending()
{
#if HAS_THREADS
	auto tasks = queue.getAll();
	for (auto& t : tasks) {
		t.task();
	}
#endif
}

void Executor::runForever()
{
	while (running)	{
		auto next = queue.getNext();
		try {
			next.task();
		} catch (std::exception& e) {
			Logger::logException(e);
			Debug::abort("Aborting due to exception while executing task " + String(next.name) + ":\n" + e.what());
		} catch (...) {
			Debug::abort("Aborting due to unknown exception while executing task " + String(next.name));
		}
	}
}

void Executor::stop()
{
#if HAS_THREADS
	running = false;
	queue.abort();
#endif
}

SingleThreadExecutor::SingleThreadExecutor(String name, MakeThread makeThread)
	: executor(queue)
{
	thread = makeThread(std::move(name), [=] ()
	{
		executor.runForever();
	});
}

SingleThreadExecutor::~SingleThreadExecutor()
{
	executor.stop();
	thread.join();
}

ExecutionQueue& SingleThreadExecutor::getQueue()
{
	return queue;
}

void SingleThreadExecutor::stop()
{
	executor.stop();
}

ThreadPool::ThreadPool(const String& name, ExecutionQueue& queue, size_t n, MakeThread makeThread)
	: name(name)
{
#if HAS_THREADS
	for (size_t i = 0; i < n; i++) {
		executors.emplace_back(std::make_unique<Executor>(queue));
	}
	threads.resize(n);

	for (size_t i = 0; i < n; i++) {
		threads[i] = makeThread(name + " Pool " + toString(i), [this, i]()
		{
			try {
				executors[i]->runForever();
			} catch (std::exception& e) {
				Logger::logException(e);
			} catch (...) {
				Logger::logError("Unknown exception in thread pool");
			}
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
