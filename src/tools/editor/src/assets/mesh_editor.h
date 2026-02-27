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

		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;

	private:
		Project& project;
		ProjectWindow& projectWindow;

		std::shared_ptr<UIScene3D> scene3d;

		Vector3f meshCentre;
		Vector3f meshSize;

		Vector2f yawAndPitch;

		bool dragging = false;
		UIInertialDrag inertialDrag;

		void updateCamera();

		void onMouseDelta(Vector2f delta);
	};
}
