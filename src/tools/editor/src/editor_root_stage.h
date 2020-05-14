#pragma once

#include "prec.h"
#include "halley/tools/dll/dynamic_library.h"
#include "halley/tools/tasks/editor_task_set.h"
#include "ui/toolbar.h"

namespace Halley {
	class HalleyEditor;
	class EditorUIFactory;
	class Project;

	enum class EditorTabs {
		Assets,
		Scene,
		ECS,
		Remotes,
		Properties,
		Settings
	};

	class EditorRootStage final : public Stage, public IDynamicLibraryListener
	{
	public:
		EditorRootStage(HalleyEditor& editor, std::unique_ptr<Project> project);
		~EditorRootStage();

		void init() override;
		void onVariableUpdate(Time time) override;
		void onRender(RenderContext& context) const override;

		void openPrefab(const String& name, AssetType assetType);
		void setPage(EditorTabs tab);
		void createLoadProjectUI();

		EditorTaskSet& getTasks() const;

		void onUnloadDLL() override;
		void onLoadDLL() override;
		
	private:
		HalleyEditor& editor;
		I18N i18n;
		Executor mainThreadExecutor;

		std::unique_ptr<Project> project;

		Sprite halleyLogo;
		Sprite background;

		std::unique_ptr<EditorUIFactory> uiFactory;
		std::unique_ptr<UIRoot> ui;

		std::shared_ptr<UIWidget> uiMainPanel;
		std::shared_ptr<UIWidget> uiTop;
		std::shared_ptr<UIWidget> uiMid;
		std::shared_ptr<UIWidget> uiBottom;
		std::shared_ptr<Toolbar> toolbar;
		std::shared_ptr<UIPagedPane> pagedPane;

		std::vector<IEditorCustomTools::ToolData> customTools;

		std::unique_ptr<EditorTaskSet> tasks;
		std::unique_ptr<DevConServer> devConServer;

		void initSprites();
		void clearUI();
		void createUI();
		void createProjectUI();
		void loadCustomProjectUI();
		void destroyCustomProjectUI();

		void updateUI(Time time);

		void loadProject();
		void unloadProject();
	};
}
