#pragma once

#include "prec.h"
#include "tasks/editor_task.h"

namespace Halley {
	class HalleyEditor;
	class ConsoleWindow;

	class EditorRootStage final : public Stage
	{
	public:
		EditorRootStage(HalleyEditor& editor);
		~EditorRootStage();

		void init() override;
		void onVariableUpdate(Time time) override;
		void onRender(RenderContext& context) const override;

	private:
		HalleyEditor& editor;

		Sprite halleyLogo;
		Sprite background;
		std::unique_ptr<ConsoleWindow> console;

		std::vector<EditorTaskAnchor> tasks;

		void initSprites();
		void updateTasks(Time time);
	};
}
