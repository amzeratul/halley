#pragma once
#include "asset_editor.h"

namespace Halley {
	class AudioObjectEditor : public AssetEditor {
    public:
        AudioObjectEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);

        void reload() override;
        void refreshAssets() override;

		bool isModified() override;
		void save() override;
        void markModified();
	
    protected:
        void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const String& assetId) override;

	private:
        bool modified = false;
        std::shared_ptr<AudioObject> audioObject;
	};
}
