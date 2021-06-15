#pragma once
#include "asset_editor.h"
#include "src/scene/scene_editor_window.h"

namespace Halley {
	class PrefabEditor : public AssetEditor {
	public:
		PrefabEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, ProjectWindow& projectWindow);

		void refreshAssets() override;
		void onDoubleClick() override;
		bool isModified() override;

	protected:
		void update(Time t, bool moved) override;
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;
		void onTabbedIn() override;
	
	private:
		Project& project;
		ProjectWindow& projectWindow;
		std::shared_ptr<SceneEditorWindow> window;

		bool pendingLoad = false;

		void open();
	};
}
