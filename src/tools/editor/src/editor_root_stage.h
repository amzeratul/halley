#pragma once

#include "prec.h"
#include "halley/tools/tasks/editor_task_set.h"
#include "ui/toolbar.h"

namespace Halley {
	class HalleyEditor;
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

		void setSoftCursor(bool enabled);
		bool isSoftCursor() const;

	private:
		HalleyEditor& editor;
		I18N i18n;
		Executor mainThreadExecutor;

		std::unique_ptr<Project> project;

		Sprite halleyLogo;
		Sprite background;
		Sprite cursor;

		std::unique_ptr<EditorUIFactory> uiFactory;
		std::unique_ptr<UIRoot> ui;
		std::shared_ptr<UIWidget> topLevelUI;

		std::unique_ptr<DevConServer> devConServer;

		bool softCursor;

		void initSprites();
		void createUI();

		void updateUI(Time time);
		void createLoadProjectUI();
		void setTopLevelUI(std::shared_ptr<UIWidget> ui);

		void loadProject();
		void unloadProject();
	};
}
