#pragma once
#include "asset_editor.h"

namespace Halley {
	class AudioObjectEditor : public AssetEditor {
    public:
        AudioObjectEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);

        void refresh();
        void reload() override;
        void refreshAssets() override;
	
    protected:
        void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const String& assetId) override;
	};
}
