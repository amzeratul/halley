#pragma once

#include "halley/tools/tasks/editor_task.h"
#include "prec.h"

namespace Halley
{
	class EditorTaskSet;

	class TaskDisplay : public UIWidget
	{
	public:
		TaskDisplay(UIFactory& factory, std::shared_ptr<EditorTaskAnchor> task);

		void update(Time t, bool moved) override;
		bool updateTask(Time time, float targetDisplaySlot);
		
		const std::shared_ptr<EditorTaskAnchor>& getTask() const { return task; }
		void setTask(std::shared_ptr<EditorTaskAnchor> task);

		void onMakeUI() override;
		float getDisplaySlot() const { return displaySlot; }

	private:
		UIFactory& factory;
		std::shared_ptr<EditorTaskAnchor> task;

		float progressDisplay = 0;
		float completeTime = 0;
		float displaySlot = -1;

		std::shared_ptr<UILabel> name;
		std::shared_ptr<UILabel> desc;
		std::shared_ptr<UIImage> bg;
		std::shared_ptr<UIImage> bgFill;
	};

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
