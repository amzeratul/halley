#pragma once

#include "prec.h"
#include "halley/tools/tasks/editor_task_set.h"

namespace Halley {
	class HalleyEditor;
	class ConsoleWindow;
	class TaskBar;
	class EditorUIFactory;
	class Project;

	class EditorRootStage final : public Stage
	{
	public:
		EditorRootStage(HalleyEditor& editor, std::unique_ptr<Project> project);
		~EditorRootStage();

		void init() override;
		void onVariableUpdate(Time time) override;
		void onRender(RenderContext& context) const override;

	private:
		HalleyEditor& editor;
		I18N i18n;

		std::unique_ptr<Project> project;

		Sprite halleyLogo;
		Sprite background;

		std::unique_ptr<EditorUIFactory> uiFactory;
		std::unique_ptr<UIRoot> ui;

		std::shared_ptr<UIWidget> uiMainPanel;
		std::shared_ptr<UIWidget> uiTop;
		std::shared_ptr<UIWidget> uiMid;
		std::shared_ptr<UIWidget> uiBottom;

		std::unique_ptr<TaskBar> taskBar;
		
		std::unique_ptr<EditorTaskSet> tasks;
		std::unique_ptr<DevConServer> devConServer;

		void initSprites();
		void clearUI();
		void createUI();
		void createLoadProjectUI();
		void createProjectUI();

		void updateUI(Time time);

		void loadProject();
	};
}
