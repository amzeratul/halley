#pragma once

#include "prec.h"

namespace Halley
{
	class TaskDetails;
	class EditorTaskSet;
	class TaskDisplay;
	
	class TaskBar : public UIWidget
	{
	public:
		TaskBar(UIFactory& factory, EditorTaskSet& taskSet, const HalleyAPI& api);
		~TaskBar();

		void update(Time time, bool moved) override;
		void draw(UIPainter& painter) const override;

		void showTaskDetails(const TaskDisplay& taskDisplay);

	private:
		UIFactory& factory;
		Resources& resources;
		EditorTaskSet& taskSet;
		const HalleyAPI& api;
		std::vector<std::shared_ptr<TaskDisplay>> tasks;

		Sprite barSolid;
		Sprite barFade;
		Sprite halleyLogo;

		float displaySize = 0;

		const TaskDisplay* taskDisplayHovered = nullptr;
		bool waitingToShowTaskDisplay = false;
		std::shared_ptr<TaskDetails> taskDetails;

		std::shared_ptr<TaskDisplay> getDisplayFor(const std::shared_ptr<EditorTaskAnchor>& task);
	};
}
