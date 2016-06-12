#include <halley/concurrency/executor.h>
#include <halley/support/exception.h>

using namespace Halley;

Executor* Executor::defaultExecutor = nullptr;

Executor::Executor()
{
	hasTasks.store(false);
}

TaskBase Executor::getNext()
{
	while (true) {
		boost::unique_lock<boost::mutex> lock(mutex);
		if (queue.empty()) {
			condition.wait(lock);
		} else {
			TaskBase value = queue.front();
			queue.pop_front();
			return value;
		}
	}
}

void Executor::addToQueue(TaskBase task)
{
	boost::unique_lock<boost::mutex> lock(mutex);
	queue.emplace_back(task);
	hasTasks.store(true);

	condition.notify_all();
}

Executor& Executor::getDefault()
{
	if (!defaultExecutor) {
		throw Exception("Default executor not specified");
	}
	return *defaultExecutor;
}

void Executor::setDefault(Executor& e)
{
	defaultExecutor = &e;
}

size_t Executor::threadCount() const
{
	return attachedCount.load();
}

void Executor::onAttached()
{
	++attachedCount;
}

void Executor::onDetached()
{
	--attachedCount;
}

ExecutorRunner::ExecutorRunner(Executor& queue)
	: queue(queue)
{
	queue.onAttached();
}

ExecutorRunner::~ExecutorRunner()
{
	queue.onDetached();
}

bool ExecutorRunner::runPending()
{
	// TODO
	return false;
}

void ExecutorRunner::runForever()
{
	running = true;
	while (running)	{
		queue.getNext()();
	}
}

void ExecutorRunner::stop()
{
	running = false;
}

void ExecutorRunner::makeThreadPool(Executor& queue, size_t n)
{
	std::reference_wrapper<Executor> q = queue;
	for (size_t i = 0; i < n; i++) {
		boost::thread([q] ()
		{
			ExecutorRunner r(q.get());
			r.runForever();
		});
	}
}
