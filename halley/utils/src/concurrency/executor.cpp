#include <halley/concurrency/concurrent.h>
#include <halley/concurrency/executor.h>
#include <halley/support/exception.h>

using namespace Halley;

class AbortException : public std::exception {};

Executors* Executors::instance = nullptr;

ExecutionQueue::ExecutionQueue()
	: aborted(false)
{
	hasTasks.store(false);
}

TaskBase ExecutionQueue::getNext()
{
	while (true) {
		boost::unique_lock<boost::mutex> lock(mutex);
		while (queue.empty()) {
			condition.wait(lock);
			if (aborted) {
				throw AbortException();
			}
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

	condition.notify_one();
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
	if (!aborted) {
		aborted = true;
		condition.notify_all();
	}
}

ExecutionQueue& ExecutionQueue::getDefault()
{
	return Executors::get().getCPU();
}

Executor::Executor(ExecutionQueue& queue)
	: queue(queue)
	, running(true)
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
	while (running)	{
		try {
			queue.getNext()();
		} catch (AbortException) {}
	}
}

void Executor::stop()
{
	running = false;
	queue.abort();
}

ThreadPool::ThreadPool(ExecutionQueue& queue, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		executors.emplace_back(std::make_unique<Executor>(queue));
	}
	threads.resize(n);

	for (size_t i = 0; i < n; i++) {
		threads[i] = boost::thread([this, i]()
		{
			Concurrent::setThreadName("threadPool" + String::integerToString(int(i)));
			executors[i]->runForever();
		});
	}
}

ThreadPool::~ThreadPool()
{
	for (auto& e: executors) {
		e->stop();
	}
	for (auto& t : threads) {
		t.join();
	}
}
