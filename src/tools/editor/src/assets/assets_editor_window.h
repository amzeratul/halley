#pragma once

#include "halley/core/api/halley_api.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class Project;
	class MetadataEditor;
	
	class AssetEditor : public UIWidget	{
	public:
        AssetEditor(UIFactory& factory, Resources& resources, Project& project, AssetType type);
		virtual ~AssetEditor() = default;

		void setResource(const String& assetId);
		void clearResource();
		virtual void reload();

	protected:
		UIFactory& factory;
		Project& project;
		Resources& resources;
		AssetType assetType;
		String assetId;
		std::shared_ptr<const Resource> resource;
	};

	class AssetsEditorWindow : public UIWidget {
    public:
        AssetsEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api);

	private:
		UIFactory& factory;
		Project& project;

		std::unique_ptr<Resources> gameResources;
		std::map<AssetType, Path> curPaths;
		Path curSrcPath;
		AssetType curType = AssetType::Sprite;

		bool assetSrcMode = true;
		std::vector<std::shared_ptr<AssetEditor>> curEditors;
        
		std::shared_ptr<UIList> assetList;
		std::shared_ptr<UIList> contentList;
		std::shared_ptr<UIPagedPane> content;
        std::shared_ptr<MetadataEditor> metadataEditor;

        void loadResources(const HalleyAPI& api);
        void makeUI();
		void setAssetSrcMode(bool enabled);

		void listAssetSources();
		void listAssets(AssetType type);
		void setListContents(std::vector<String> files, const Path& curPath);
		void refreshList();

		void loadAsset(const String& name);
		void refreshAssets(const std::vector<String>& assets);

		std::shared_ptr<AssetEditor> makeEditor(AssetType type, const String& name);
		void createEditorTab(AssetType type, const String& name);
    };
}
