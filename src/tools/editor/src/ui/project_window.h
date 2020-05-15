#pragma once

#include "halley/tools/dll/dynamic_library.h"
#include "halley/ui/ui_widget.h"
#include "src/editor_root_stage.h"

namespace Halley {
    class UIFactory;
    class HalleyEditor;
	class Toolbar;
	class Project;

    class ProjectWindow final : public UIWidget, public IDynamicLibraryListener {
    public:
        ProjectWindow(UIFactory& factory, HalleyEditor& editor, Project& project, Resources& resources, const HalleyAPI& api);
    	~ProjectWindow();

        void setPage(EditorTabs tab);
        String setCustomPage(const String& pageId);
        void openPrefab(const String& assetId, AssetType assetType);

    	EditorTaskSet& getTasks() const;

    protected:
		void onUnloadDLL() override;
        void onLoadDLL() override;

        void update(Time t, bool moved) override;
    	
    private:
		constexpr static int numOfStandardTools = 6;

    	UIFactory& factory;
        HalleyEditor& editor;
    	Project& project;
    	Resources& resources;
    	const HalleyAPI& api;

		std::shared_ptr<UIWidget> uiTop;
		std::shared_ptr<UIWidget> uiMid;
		std::shared_ptr<UIWidget> uiBottom;
		std::shared_ptr<Toolbar> toolbar;
		std::shared_ptr<UIPagedPane> pagedPane;

		std::unique_ptr<EditorTaskSet> tasks;

		std::vector<IEditorCustomTools::ToolData> customTools;
    	
		void makeUI();
    	void makeToolbar();
    	void makePagedPane();
		void loadCustomUI();
		void destroyCustomUI();
    };
}
