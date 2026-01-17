#pragma once
#include "asset_editor.h"
#include "halley/plugin/iasset_importer.h"
#include "src/scene/scene_editor_window.h"

namespace Halley {
	class MeshEditor : public AssetEditor, private IAddionalFileReader {
	public:
		MeshEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, ProjectWindow& projectWindow);

		void refreshAssets() override;
		bool isReadyToLoad() const override;

	protected:
		void update(Time t, bool moved) override;
		std::shared_ptr<const Resource> loadResource(const Path& assetPath, const String& assetId, AssetType assetType) override;
		void onTabbedIn() override;

		Bytes readAdditionalFile(const Path& filePath) override;

	private:
		Project& project;
		ProjectWindow& projectWindow;

		std::shared_ptr<UIScene3D> scene3d;
	};
}
