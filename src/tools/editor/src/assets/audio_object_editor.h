#pragma once
#include "asset_editor.h"
#include "halley/audio/audio_sub_object.h"

namespace Halley {
    class AudioObjectEditorTreeList;

	class AudioObjectEditor : public AssetEditor {
    public:
        AudioObjectEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);

        void reload() override;
        void refreshAssets() override;

		bool isModified() override;
		void save() override;
        void markModified();

        void onMakeUI() override;

        bool canParentItemTo(const String& itemId, const String& parentId) const;
	
    protected:
        void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const String& assetId) override;

	private:
        struct TreeData {
	        AudioSubObjectType type;
            bool subObject = false;
            bool clip = false;
        };

		bool modified = false;
        std::shared_ptr<AudioObject> audioObject;
        std::shared_ptr<AudioObjectEditorTreeList> hierarchy;
        HashMap<String, TreeData> treeData;

        void doLoadUI();
        void populateObject(const String& parentId, size_t idx, const AudioSubObjectHandle& subObject);

		Sprite makeIcon(AudioSubObjectType type) const;
	};

    class AudioObjectEditorTreeList : public UITreeList {
    public:
        AudioObjectEditorTreeList(String id, UIStyle style);

    	void setParent(AudioObjectEditor& parent);

    protected:
        bool canParentItemTo(const String& itemId, const String& parentId) const override;

    private:
        AudioObjectEditor* parent = nullptr;
    };
}
