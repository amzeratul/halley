#include <halley/concurrency/concurrent.h>
#include <halley/concurrency/executor.h>
#include <halley/support/exception.h>

using namespace Halley;

Executors* Executors::instance = nullptr;

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

std::vector<TaskBase> ExecutionQueue::getAll()
{
	boost::unique_lock<boost::mutex> lock(mutex);
	hasTasks.store(false);
	std::vector<TaskBase> tasks(queue.begin(), queue.end());
	queue.clear();
	return tasks;
}

void ExecutionQueue::addToQueue(TaskBase task)
{
	boost::unique_lock<boost::mutex> lock(mutex);
	queue.emplace_back(task);
	hasTasks.store(true);

	condition.notify_all();
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

ExecutionQueue& ExecutionQueue::getDefault()
{
	return Executors::get().getCPU();
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
	auto tasks = queue.getAll();
	for (auto& t : tasks) {
		t();
	}
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
		}).detach();
	}
}
