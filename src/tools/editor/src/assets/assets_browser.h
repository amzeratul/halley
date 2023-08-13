#pragma once

#include "asset_browser_tabs.h"
#include "halley/api/halley_api.h"
#include "halley/tools/project/project.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_list.h"
#include "halley/ui/widgets/ui_paged_pane.h"

namespace Halley {
	class EditorUIFactory;
	class ProjectWindow;
	class Project;
	class MetadataEditor;
	class AssetEditor;
	class AssetEditorWindow;
	
	class AssetsBrowser : public UIWidget, public Project::IAssetSrcChangeListener {
    public:
        AssetsBrowser(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow);

		void update(Time t, bool moved) override;

		void openAsset(AssetType type, const String& assetId);
		void openFile(const Path& path);
		void showFile(const Path& path);
        void replaceAssetTab(AssetType oldType, const String& oldId, AssetType newType, const String& newId);

		bool requestQuit(std::function<void()> callback);
        void saveTab();
		void saveAllTabs();
		void closeTab();
        void moveTabFocus(int delta);

		std::shared_ptr<AssetEditorWindow> getActiveWindow() const;

		void onAssetsSrcChanged() override;

    private:
		EditorUIFactory& factory;
		Project& project;
		ProjectWindow& projectWindow;

		std::map<AssetType, Path> curPaths;
		Path curSrcPath;
		AssetType curType = AssetType::Sprite;

		Vector<String> assetNames;
		std::optional<Path> pendingOpen;
        
		std::shared_ptr<UIList> assetList;
		std::shared_ptr<AssetBrowserTabs> assetTabs;

        String lastClickedAsset;

		uint64_t curHash = 0;
		uint64_t curDirHash = 0;

		bool collapsed = false;
        int waitingToShowSel = 0;

        void loadResources();
        void makeUI();
		void setAssetSrcMode(bool enabled);
		void updateAddRemoveButtons();

		void listAssetSources();
		void listAssets(AssetType type);
		void setListContents();
		void refreshList();

		void clearAssetList();
		void addDirToList(const Path& curPath, const String& dir);
		void addFileToList(const Path& path);

		void setSelectedAsset(const String& name);
		void loadAsset(const String& name);
		void refreshAssets(gsl::span<const String> assets);

		void openContextMenu(const String& assetId);
		void onContextMenuAction(const String& assetId, const String& action);

		void addAsset();
		void addAsset(Path path, std::string_view data, bool fullPath = false);
		void removeAsset();
		void removeAsset(const String& assetId);
		void renameAsset(const String& oldName, const String& newName);
		void removeFolder(const String& assetId);
		void renameFolder(const String& oldName, const String& newName);
		void duplicateAsset(const String& srcId, const String& dstId);
		void addFolder();
		void addFolder(Path path);

		void setCollapsed(bool collapsed);
		void doSetCollapsed(bool collapsed);
	};
}
