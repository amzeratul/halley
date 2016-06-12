#include <halley/concurrency/executor.h>
#include <halley/support/exception.h>

using namespace Halley;

Executor* Executor::defaultExecutor = nullptr;

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

Executor* Executor::createDefault()
{
	defaultExecutor = new Executor();

	boost::thread([] ()
	{
		defaultExecutor->runForever();
	});

	return defaultExecutor;
}

Executor::Executor()
{
	hasTasks.store(false);
}

void Executor::addToQueue(TaskBase task)
{
	boost::unique_lock<boost::mutex> lock(mutex);
	queue.emplace_back(task);
	hasTasks.store(true);

	condition.notify_one();
}

bool Executor::runPending()
{
	std::vector<TaskBase> toRun;
	{
		boost::unique_lock<boost::mutex> lock(mutex);
		toRun = std::move(queue);
		hasTasks.store(false);
	}

	for (auto t: toRun)
	{
		t();
	}

	return hasTasks.load();
}

void Executor::runForever()
{
	running = true;
	do {
		while(runPending()) {}

		boost::unique_lock<boost::mutex> lock(mutex);
		condition.wait(lock);
	} while (running);
}

void Executor::stop()
{
	running = false;
}

size_t Executor::threadCount() const
{
	return 1;
}
