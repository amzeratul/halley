#pragma once
#include "asset_editor.h"

namespace Halley {
	class PrefabEditor : public AssetEditor {
	public:
		PrefabEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, ProjectWindow& projectWindow);

		void reload() override;
		void onDoubleClick() override;

	protected:
		void update(Time t, bool moved) override;
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;
		
	private:
		Project& project;
		ProjectWindow& projectWindow;

		void setupWindow();
		void open();
		void updateButton();
	};
}
