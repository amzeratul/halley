#pragma once
#include "asset_editor.h"
#include "halley/audio/audio_sub_object.h"

namespace Halley {
	class AudioObjectEditor : public AssetEditor {
    public:
        AudioObjectEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);

        void reload() override;
        void refreshAssets() override;

		bool isModified() override;
		void save() override;
        void markModified();

        void onMakeUI() override;
	
    protected:
        void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const String& assetId) override;

	private:
        bool modified = false;
        std::shared_ptr<AudioObject> audioObject;
        std::shared_ptr<UITreeList> hierarchy;

        void doLoadUI();
        void populateObject(const String& parentId, size_t idx, const AudioSubObjectHandle& subObject);

		Sprite makeIcon(AudioSubObjectType type) const;
	};
}
