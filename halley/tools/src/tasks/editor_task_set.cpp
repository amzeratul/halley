#include "halley/tools/tasks/editor_task_set.h"
#include <thread>
#include <chrono>

using namespace Halley;
using namespace std::chrono_literals;

EditorTaskSet::EditorTaskSet() 
{}

EditorTaskSet::~EditorTaskSet()
{
	// We're not going anywhere until all those tasks are done; cancel them all to speed it up
	for (auto& t : tasks) {
		t.cancel();
	}

	// Keep updating until they're all cancelled
	while (!tasks.empty()) {
		std::this_thread::sleep_for(25ms);
		update(0.025f);
	}
}

void EditorTaskSet::update(Time time)
{
	for (size_t i = 0; i < tasks.size(); ) {
		tasks[i].update(static_cast<float>(time));

		auto subTasks = std::move(tasks[i].getPendingTasks());
		for (auto& t : subTasks) {
			addTask(std::move(t));
		}

		if (tasks[i].getStatus() == EditorTaskStatus::Done) {
			auto newTasks = std::move(tasks[i].getContinuations());
			for (auto& t : newTasks) {
				addTask(std::move(t));
			}
			tasks.erase(tasks.begin() + i);
		} else {
			++i;
		}
	}
}

void EditorTaskSet::addTask(EditorTaskAnchor&& task)
{
	tasks.emplace_back(std::move(task));
	tasks.back().setId(nextId++);
}
