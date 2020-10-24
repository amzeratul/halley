#pragma once

#include "halley/core/api/halley_api.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_list.h"
#include "halley/ui/widgets/ui_paged_pane.h"

namespace Halley {
	class ProjectWindow;
	class Project;
	class MetadataEditor;
	class AssetEditor;
	
	class AssetsBrowser : public UIWidget {
    public:
        AssetsBrowser(UIFactory& factory, Project& project, ProjectWindow& projectWindow);
        void showAsset(AssetType type, const String& assetId);
		void showFile(const Path& path);

	private:
		UIFactory& factory;
		Project& project;
		ProjectWindow& projectWindow;

		std::map<AssetType, Path> curPaths;
		Path curSrcPath;
		AssetType curType = AssetType::Sprite;

		bool assetSrcMode = true;
		std::vector<std::shared_ptr<AssetEditor>> curEditors;
		std::optional<std::vector<String>> assetNames;
		String filter;
        
		std::shared_ptr<UIList> assetList;
		std::shared_ptr<UIList> contentList;
		std::shared_ptr<UIDropdown> contentListDropdown;
		std::shared_ptr<UILabel> contentListDropdownLabel;
		std::shared_ptr<UIPagedPane> content;
        std::shared_ptr<MetadataEditor> metadataEditor;

		String loadedAsset;
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

		void loadAsset(const String& name, bool doubleClick, bool clearDropdown);
		void refreshAssets(const std::vector<String>& assets);
		void onDoubleClickAsset();

		std::shared_ptr<AssetEditor> makeEditor(Path filePath, AssetType type, const String& name);
		void createEditorTab(Path filePath, AssetType type, const String& name);

		void addAsset();
		void removeAsset();

		Path getCurrentAssetPath() const;
		void openFileExternally(const Path& path);
		void showFileExternally(const Path& path);
	};
}
