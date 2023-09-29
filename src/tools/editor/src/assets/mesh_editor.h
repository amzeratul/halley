#pragma once

#include "asset_editor.h"

namespace Halley {
    class MeshEditorDisplay;

    class MeshEditor : public AssetEditor {
	public:
		MeshEditor(UIFactory& factory, Resources& gameResources, AssetType type, Project& project, MetadataEditor& metadataEditor);

		void reload() override;
		void refreshAssets() override;

	protected:
		void update(Time t, bool moved) override;
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;

	private:
		MetadataEditor& metadataEditor;
		std::shared_ptr<MeshEditorDisplay> meshDisplay;

		void setupWindow();
		void loadAssetData();

	};

	class MeshEditorDisplay : public UIWidget {
	public:
		MeshEditorDisplay(String id, Resources& resources, const HalleyAPI& api);

		void setMetadataEditor(MetadataEditor& metadataEditor);
		void setMesh(std::shared_ptr<const Mesh> mesh);

	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;
		void render(RenderContext& rc) const override;
		

	private:
		Resources& resources;
		const HalleyAPI& api;
		MetadataEditor* metadataEditor = nullptr;

	    std::shared_ptr<MeshRenderer> meshRenderer = nullptr;
		std::shared_ptr<RenderSurface> surface = nullptr;


		// TODO TEMP
		Time curTime = 0.0;
	};

}