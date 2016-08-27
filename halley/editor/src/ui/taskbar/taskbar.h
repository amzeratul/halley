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
			String label;
			String subLabel;
			float progress = 0;
			float progressDisplay = 0;
			bool running = false;
			int id = 0;
			float completeTime = 0;
			float displaySlot = -1;

			std::shared_ptr<Material> material;
		};

	public:
		TaskBar(Resources& resources);
		void update(const std::vector<EditorTaskAnchor>& tasks, Time time);
		void draw(Painter& painter);

	private:
		Sprite barSolid;
		Sprite barFade;
		Sprite halleyLogo;
		Sprite taskSprite;

		std::shared_ptr<Material> taskMaterial;
		std::shared_ptr<Font> font;

		std::vector<TaskDisplay> tasks;

		float displaySize = 0;

		TaskDisplay& getDisplayForId(int id);
	};
}
