#pragma once

#include "asset_browser_tabs.h"
#include "halley/core/api/halley_api.h"
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
	
	class AssetsBrowser : public UIWidget {
    public:
        AssetsBrowser(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow);
        void showAsset(AssetType type, const String& assetId);
		void showFile(const Path& path);

    private:
		EditorUIFactory& factory;
		Project& project;
		ProjectWindow& projectWindow;

		std::map<AssetType, Path> curPaths;
		Path curSrcPath;
		AssetType curType = AssetType::Sprite;

		bool assetSrcMode = true;
		std::optional<std::vector<String>> assetNames;

		FuzzyTextMatcher fuzzyMatcher;
		String filter;
        
		std::shared_ptr<UIList> assetList;
		std::shared_ptr<AssetBrowserTabs> assetTabs;

        String lastClickedAsset;

		uint64_t curHash = 0;
		uint64_t curDirHash = 0;

        void loadResources();
        void makeUI();
		void setAssetSrcMode(bool enabled);
		void updateAddRemoveButtons();

		void listAssetSources();
		void listAssets(AssetType type);
		void setListContents(std::vector<String> files, const Path& curPath, bool flat);
		void refreshList();
		void setFilter(const String& filter);

		void clearAssetList();
		void addDirToList(const String& dir);
		void addFileToList(const Path& path);

		void loadAsset(const String& name, bool doubleClick);
		void refreshAssets(const std::vector<String>& assets);

		void addAsset();
		void removeAsset();
	};
}
