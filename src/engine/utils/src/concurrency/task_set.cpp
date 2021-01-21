#include "halley/concurrency/task_set.h"
#include <thread>
#include <chrono>
#include <iostream>

#include "halley/concurrency/task_anchor.h"

using namespace Halley;
using namespace std::chrono_literals;

TaskSet::TaskSet() 
{}

TaskSet::~TaskSet()
{
	// We're not going anywhere until all those tasks are done; cancel them all to speed it up
	for (auto& t : tasks) {
		t->cancel();
	}

	// Keep updating until they're all cancelled
	while (!tasks.empty()) {
		for (auto& t : tasks) {
			std::cout << "Waiting for: " << t->getName() << std::endl;
		}
		std::this_thread::sleep_for(25ms);
		update(0.025f);
	}
}

void TaskSet::update(Time time)
{
	Vector<std::unique_ptr<Task>> toAdd;

	auto next = tasks.begin();
	for (auto iter = tasks.begin(); iter != tasks.end(); iter = next) {
		++next;

		auto& task = *iter;
		task->update(static_cast<float>(time));

		auto subTasks = task->getPendingTasks();
		for (auto& t : subTasks) {
			toAdd.push_back(std::move(t));
		}

		if (task->getStatus() == TaskStatus::Done) {
			auto newTasks = task->getContinuations();
			for (auto& t : newTasks) {
				toAdd.push_back(std::move(t));
			}
			task->terminate();
			if (listener) {
				if (task->hasError()) {
					listener->onTaskError(task);
				}
				listener->onTaskTerminated(task);
			}
			tasks.erase(iter);
		}
	}

	for (auto& t: toAdd) {
		addTask(std::move(t));
	}
}

void TaskSet::addTask(std::shared_ptr<TaskAnchor> task)
{
	tasks.emplace_back(std::move(task));
	tasks.back()->setId(nextId++);
	if (listener) {
		listener->onTaskAdded(tasks.back());
	}
}

void TaskSet::addTask(std::unique_ptr<Task> task)
{
	addTask(std::make_shared<TaskAnchor>(std::move(task)));
}

void TaskSet::setListener(TaskSetListener& l)
{
	listener = &l;
}

const std::list<std::shared_ptr<TaskAnchor>>& TaskSet::getTasks() const
{
	return tasks;
}
