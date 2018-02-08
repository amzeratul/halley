#include "halley/tools/tasks/editor_task_set.h"
#include <thread>
#include <chrono>
#include <iostream>

using namespace Halley;
using namespace std::chrono_literals;

EditorTaskSet::EditorTaskSet() 
{}

EditorTaskSet::~EditorTaskSet()
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

void EditorTaskSet::update(Time time)
{
	auto next = tasks.begin();
	for (auto iter = tasks.begin(); iter != tasks.end(); iter = next) {
		++next;

		auto& task = *iter;
		task->update(static_cast<float>(time));

		auto subTasks = task->getPendingTasks();
		for (auto& t : subTasks) {
			addTask(std::move(t));
		}

		if (task->getStatus() == EditorTaskStatus::Done) {
			auto newTasks = task->getContinuations();
			for (auto& t : newTasks) {
				addTask(std::move(t));
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
}

void EditorTaskSet::addTask(EditorTaskAnchor&& task)
{
	task.setId(nextId++);
	tasks.emplace_back(std::make_shared<EditorTaskAnchor>(std::move(task)));
	if (listener) {
		listener->onTaskAdded(tasks.back());
	}
}

void EditorTaskSet::setListener(EditorTaskSetListener& l)
{
	listener = &l;
}

const std::list<std::shared_ptr<EditorTaskAnchor>>& EditorTaskSet::getTasks() const
{
	return tasks;
}
