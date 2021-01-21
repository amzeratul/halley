#pragma once

#include "console_window.h"
#include "halley/tools/dll/dynamic_library.h"
#include "halley/ui/ui_widget.h"
#include "src/editor_root_stage.h"
#include "halley/tools/project/project.h"
#include "src/assets/assets_browser.h"
#include "src/scene/entity_editor.h"
#include "src/scene/choose_asset_window.h"

namespace Halley {
    class UIFactory;
    class HalleyEditor;
	class Toolbar;
	class Project;

    class ProjectWindow final : public UIWidget, public IDynamicLibraryListener, public Project::IAssetLoadListener {
    public:
        ProjectWindow(EditorUIFactory& factory, HalleyEditor& editor, Project& project, Resources& resources, const HalleyAPI& api);
    	~ProjectWindow();

        void setPage(EditorTabs tab);
        LocalisedString setCustomPage(const String& pageId);
    	void openFile(const String& assetId);
    	void openAsset(AssetType type, const String& assetId);

    	const HalleyAPI& getAPI() const;

    	TaskSet& getTasks() const;

    	void reloadProject();

    protected:
		void onUnloadDLL() override;
        void onLoadDLL() override;

        void onAssetsLoaded() override;

        void update(Time t, bool moved) override;
        bool onKeyPress(KeyboardKeyPress key) override;
    	
    private:
		constexpr static int numOfStandardTools = 6;

    	EditorUIFactory& factory;
        HalleyEditor& editor;
    	Project& project;
    	Resources& resources;
    	const HalleyAPI& api;

		std::shared_ptr<UIWidget> uiTop;
		std::shared_ptr<UIWidget> uiMid;
		std::shared_ptr<UIWidget> uiBottom;
		std::shared_ptr<Toolbar> toolbar;
		std::shared_ptr<UIPagedPane> pagedPane;

		std::unique_ptr<TaskSet> tasks;

		std::vector<IEditorCustomTools::ToolData> customTools;
    	bool waitingToLoadCustomUI = true;
    	bool hasAssets = false;
    	bool hasDLL = false;

    	std::shared_ptr<AssetsBrowser> assetEditorWindow;
        std::shared_ptr<ConsoleWindow> consoleWindow;
        std::shared_ptr<ChooseImportAssetWindow> assetFinder;

        void makeUI();
    	void makeToolbar();
    	void makePagedPane();

    	void tryLoadCustomUI();
    	bool loadCustomUI();
		void destroyCustomUI();

    	void openAssetFinder();
    };
}
