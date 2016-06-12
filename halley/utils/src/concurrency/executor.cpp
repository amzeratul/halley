#include <halley/concurrency/concurrent.h>
#include <halley/concurrency/executor.h>
#include <halley/support/exception.h>

using namespace Halley;

ExecutionQueue* ExecutionQueue::defaultQueue = nullptr;

ExecutionQueue::ExecutionQueue()
{
	hasTasks.store(false);
}

TaskBase ExecutionQueue::getNext()
{
	while (true) {
		boost::unique_lock<boost::mutex> lock(mutex);
		while (queue.empty()) {
			condition.wait(lock);
		}
		
		TaskBase value = queue.front();
		queue.pop_front();
		return value;
	}
}

void ExecutionQueue::addToQueue(TaskBase task)
{
	boost::unique_lock<boost::mutex> lock(mutex);
	queue.emplace_back(task);
	hasTasks.store(true);

	condition.notify_all();
}

ExecutionQueue& ExecutionQueue::getDefault()
{
	if (!defaultQueue) {
		throw Exception("Default executor not specified");
	}
	return *defaultQueue;
}

void ExecutionQueue::setDefault(ExecutionQueue& e)
{
	defaultQueue = &e;
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

Executor::Executor(ExecutionQueue& queue)
	: queue(queue)
{
	queue.onAttached();
}

Executor::~Executor()
{
	queue.onDetached();
}

bool Executor::runPending()
{
	// TODO
	return false;
}

void Executor::runForever()
{
	running = true;
	while (running)	{
		queue.getNext()();
	}
}

void Executor::stop()
{
	running = false;
}

void Executor::makeThreadPool(ExecutionQueue& queue, size_t n)
{
	std::reference_wrapper<ExecutionQueue> q = queue;
	for (size_t i = 0; i < n; i++) {
		boost::thread([q, i] ()
		{
			Concurrent::setThreadName("threadPool" + String::integerToString(int(i)));
			Executor r(q.get());
			r.runForever();
		});
	}
}
