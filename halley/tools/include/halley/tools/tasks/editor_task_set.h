#pragma once
#include "editor_task.h"
#include "halley/time/halleytime.h"

namespace Halley
{
	class EditorTaskSet
	{
	public:
		EditorTaskSet();
		~EditorTaskSet();
		
		void update(Time time);
		void addTask(EditorTaskAnchor&& editorTaskAnchor);

		const std::vector<EditorTaskAnchor>& getTasks() const { return tasks; }
	
	private:
		std::vector<EditorTaskAnchor> tasks;
		int nextId = 0;
	};
}
