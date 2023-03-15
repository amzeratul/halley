#pragma once
#include "task.h"
#include "halley/time/halleytime.h"
#include <list>

#include "halley/data_structures/hash_map.h"

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

	class TaskSet;

	class TaskExclusivityHandle {
		friend class TaskSet;

	public:
		TaskExclusivityHandle(TaskSet& parent, Vector<String> tags);
		~TaskExclusivityHandle();

	private:
		TaskSet& parent;
		Vector<String> tags;
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

		std::pair<std::unique_ptr<TaskExclusivityHandle>, String> getExclusiveHandle(const String& taskName, const Vector<String>& tags);
		void returnHandle(TaskExclusivityHandle& handle);

	private:
		std::list<std::shared_ptr<TaskAnchor>> tasks;
		TaskSetListener* listener = nullptr;
		int nextId = 0;

		HashMap<String, String> exclusiveClaims;
	};
}
