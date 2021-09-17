#pragma once

#include "halley/core/api/halley_api.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class ProjectWindow;
	class Project;
	class MetadataEditor;
	
	class AssetEditor : public UIWidget	{
	public:
        AssetEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type);
		virtual ~AssetEditor() = default;

        void update(Time t, bool moved) override;

		void setResource(const String& assetId);
		void clearResource();
		virtual void reload();
		virtual void refreshAssets();
        virtual void onDoubleClick();
        virtual bool isModified();
        virtual void save();

    protected:
		virtual std::shared_ptr<const Resource> loadResource(const String& assetId) = 0;

		virtual void onTabbedIn();
		void setTabbedIn(bool value);
		void tryLoading();
		void load();

		UIFactory& factory;
		Project& project;
		Resources& gameResources;
		AssetType assetType;
		String assetId;
		std::shared_ptr<const Resource> resource;
		bool needsLoading = false;
		bool tabbedIn = false;
	};
}
