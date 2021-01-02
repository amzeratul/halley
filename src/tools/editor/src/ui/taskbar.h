#pragma once

#include "halley/tools/tasks/editor_task.h"
#include "prec.h"

namespace Halley
{
	class EditorTaskSet;
	class TaskDisplay;
	
	class TaskBar : public UIWidget
	{
	public:
		TaskBar(UIFactory& factory, EditorTaskSet& taskSet);
		void update(Time time, bool moved) override;
		void draw(UIPainter& painter) const override;

	private:
		UIFactory& factory;
		Resources& resources;
		EditorTaskSet& taskSet;
		std::vector<std::shared_ptr<TaskDisplay>> tasks;

		Sprite barSolid;
		Sprite barFade;
		Sprite halleyLogo;

		float displaySize = 0;

		std::shared_ptr<TaskDisplay> getDisplayFor(const std::shared_ptr<EditorTaskAnchor>& task);
	};
}
