#pragma once
#include "editor_task.h"
#include "halley/time/halleytime.h"
#include <list>

namespace Halley
{
	class EditorTaskSet
	{
	public:
		EditorTaskSet();
		~EditorTaskSet();
		
		void update(Time time);
		void addTask(EditorTaskAnchor&& editorTaskAnchor);

		const std::list<EditorTaskAnchor>& getTasks() const { return tasks; }
	
	private:
		std::list<EditorTaskAnchor> tasks;
		int nextId = 0;
	};
}
