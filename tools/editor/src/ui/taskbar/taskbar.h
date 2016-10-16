#pragma once

#include "halley/tools/tasks/editor_task.h"
#include "prec.h"

namespace Halley
{
	class TaskBar
	{
		class TaskDisplay
		{
		public:
			std::shared_ptr<EditorTaskAnchor> task;

			float progressDisplay = 0;
			float completeTime = 0;
			float displaySlot = -1;

			std::shared_ptr<Material> material;
		};

	public:
		TaskBar(Resources& resources);
		void update(const std::list<std::shared_ptr<EditorTaskAnchor>>& tasks, Time time);
		void draw(Painter& painter);

	private:
		Sprite barSolid;
		Sprite barFade;
		Sprite halleyLogo;

		std::shared_ptr<const Texture> roundRectTexture;
		std::shared_ptr<Material> taskMaterial;
		std::shared_ptr<const Font> font;

		std::vector<TaskDisplay> tasks;

		float displaySize = 0;

		TaskDisplay& getDisplayFor(const std::shared_ptr<EditorTaskAnchor>& task);
	};
}
