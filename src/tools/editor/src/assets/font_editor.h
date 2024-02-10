#pragma once

#include "asset_editor.h"

namespace Halley {
	class Project;

	class FontEditor : public AssetEditor {
    public:
        FontEditor(UIFactory& factory, Resources& gameResources, AssetType type, Project& project, ProjectWindow& projectWindow);

        void onMakeUI() override;

        void refresh();
        void reload() override;
        void refreshAssets() override;

		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;
		bool onKeyPress(KeyboardKeyPress key) override;
	
    protected:
        void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const String& assetId) override;
		
	private:
        void updatePreviews();
	};
}
