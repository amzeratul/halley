#pragma once
#include "task.h"
#include "halley/time/halleytime.h"
#include <list>

namespace Halley
{
	class TaskSetListener
	{
	public:
		virtual ~TaskSetListener() {}
		virtual void onTaskAdded(const std::shared_ptr<TaskAnchor>& task) = 0;
		virtual void onTaskTerminated(const std::shared_ptr<TaskAnchor>& task) = 0;
		virtual void onTaskError(const std::shared_ptr<TaskAnchor>& task) = 0;
	};

	class TaskSet
	{
	public:
		TaskSet();
		~TaskSet();
		
		void update(Time time);
		void addTask(std::shared_ptr<TaskAnchor> editorTaskAnchor);
		void addTask(std::unique_ptr<Task> task);

		void setListener(TaskSetListener& listener);

		const std::list<std::shared_ptr<TaskAnchor>>& getTasks() const;

	private:
		std::list<std::shared_ptr<TaskAnchor>> tasks;
		TaskSetListener* listener = nullptr;
		int nextId = 0;
	};
}
