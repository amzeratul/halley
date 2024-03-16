#pragma once

#include "halley/api/halley_api.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class ProjectWindow;
	class Project;
	class MetadataEditor;
	class PaletteWindow;
	
	class AssetEditor : public UIWidget	{
	public:
        AssetEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type);
		virtual ~AssetEditor() = default;

        void update(Time t, bool moved) override;

		void setResource(Path filePath, String assetId);
		void clearResource();
		virtual void reload();
		virtual void refreshAssets();
        virtual void onDoubleClick();
        virtual bool isModified();
        virtual void save();
		virtual bool canSave(bool forceInstantCheck) const;
        virtual void onOpenAssetFinder(PaletteWindow& assetFinder);

    protected:
		virtual std::shared_ptr<const Resource> loadResource(const Path& assetPath, const String& assetId, AssetType assetType) = 0;

		virtual void onTabbedIn();
		void setTabbedIn(bool value);
		void tryLoading();
		void load();

		UIFactory& factory;
		Project& project;
		Resources& gameResources;
		AssetType assetType;
		Path assetPath;
		String assetId;
		std::shared_ptr<const Resource> resource;
		bool needsLoading = false;
		bool tabbedIn = false;
	};

	class DrillDownAssetWindow : public UIWidget {
	public:
		DrillDownAssetWindow(String id, Vector2f minSize, UISizer sizer) : UIWidget(std::move(id), minSize, std::move(sizer)) {}
		virtual bool isModified() { return false; }
		virtual void drillDownSave() {}
	};
}
