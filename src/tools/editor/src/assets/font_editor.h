#pragma once

#include "asset_editor.h"
#include "halley/tools/assets/import_assets_database.h"

namespace Halley {
	class Project;

	class FontEditor : public AssetEditor {
    public:
        FontEditor(UIFactory& factory, Resources& gameResources, AssetType type, Project& project, ProjectWindow& projectWindow);

        void onMakeUI() override;

        void onResourceLoaded() override;
        void refreshAssets() override;
        bool isReadyToLoad() const override;
        
    protected:
        std::shared_ptr<const Resource> loadResource(const Path& assetPath, const String& assetId, AssetType assetType) override;
		
	private:
        Metadata metadata;
        Path metaPath;
        std::shared_ptr<const Font> font;
        LocalisedString previewText;

        void updateFont();
        void updatePreviews();
        void updatePreviewText();
        void updatePreviewText(LocalisedString text);

		void loadMetadata();
        void saveMetadata();
	};
}
