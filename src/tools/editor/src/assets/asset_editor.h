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
        AssetEditor(UIFactory& factory, Resources& resources, Project& project, AssetType type);
		virtual ~AssetEditor() = default;

		void setResource(const String& assetId);
		void clearResource();
		virtual void reload();
		virtual void refreshAssets();
        virtual void onDoubleClick();
        virtual bool isModified();

    protected:
		virtual std::shared_ptr<const Resource> loadResource(const String& assetId) = 0;

		UIFactory& factory;
		Project& project;
		Resources& gameResources;
		AssetType assetType;
		String assetId;
		std::shared_ptr<const Resource> resource;
	};
}
