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
		void save() override;
		bool canSave(bool forceInstantCheck) const override;
		void onOpenAssetFinder(PaletteWindow& assetFinder) override;

		void drillDownEditor(std::shared_ptr<DrillDownAssetWindow> editor);

	protected:
		void update(Time t, bool moved) override;
		std::shared_ptr<const Resource> loadResource(const Path& assetPath, const String& assetId, AssetType assetType) override;
		void onTabbedIn() override;
	
	private:
		constexpr static Time minLoadTime = 0;

		Project& project;
		ProjectWindow& projectWindow;
		std::shared_ptr<SceneEditorWindow> window;
		Vector<std::shared_ptr<DrillDownAssetWindow>> drillDown;

		bool pendingLoad = false;
		Time elapsedTime = 0;

		void open();
	};
}
