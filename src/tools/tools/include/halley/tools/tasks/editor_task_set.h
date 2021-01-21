#pragma once
#include "editor_task.h"
#include "halley/time/halleytime.h"
#include <list>

namespace Halley
{
	class EditorTaskSetListener
	{
	public:
		virtual ~EditorTaskSetListener() {}
		virtual void onTaskAdded(const std::shared_ptr<EditorTaskAnchor>& task) = 0;
		virtual void onTaskTerminated(const std::shared_ptr<EditorTaskAnchor>& task) = 0;
		virtual void onTaskError(const std::shared_ptr<EditorTaskAnchor>& task) = 0;
	};

	class EditorTaskSet
	{
	public:
		EditorTaskSet();
		~EditorTaskSet();
		
		void update(Time time);
		void addTask(std::shared_ptr<EditorTaskAnchor> editorTaskAnchor);
		void addTask(std::unique_ptr<EditorTask> task);

		void setListener(EditorTaskSetListener& listener);

		const std::list<std::shared_ptr<EditorTaskAnchor>>& getTasks() const;

	private:
		std::list<std::shared_ptr<EditorTaskAnchor>> tasks;
		EditorTaskSetListener* listener = nullptr;
		int nextId = 0;
	};
}
