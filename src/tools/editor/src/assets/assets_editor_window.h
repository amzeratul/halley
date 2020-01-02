#pragma once

#include "halley/core/api/halley_api.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class Project;
	
	class AssetEditor : public UIWidget	{
	public:
        AssetEditor(UIFactory& factory, Resources& resources, const String& assetId, AssetType type, Project& project);
		virtual ~AssetEditor() = default;

		virtual void reload();

	protected:
		UIFactory& factory;
		Project& project;
		String assetId;
		AssetType assetType;
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
		AssetType curType = AssetType::Sprite;

		std::shared_ptr<AssetEditor> curEditor;

		void makeUI();
		void loadResources(const HalleyAPI& api);
		void listAssets(AssetType type);
		void loadAsset(const String& name);
		void refreshAssets(const std::vector<String>& assets);
		std::shared_ptr<AssetEditor> createEditor(AssetType type, const String& name);
    };
}
