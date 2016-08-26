#pragma once
#include "editor_task.h"

namespace Halley
{
	class EditorTaskSet
	{
	public:
		EditorTaskSet();
		~EditorTaskSet();
		
		void update(Time time);
		void addTask(EditorTaskAnchor&& editorTaskAnchor);
	
		private:
		std::vector<EditorTaskAnchor> tasks;
	};
}
