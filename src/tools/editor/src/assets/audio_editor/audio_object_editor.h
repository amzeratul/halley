#pragma once
#include "../asset_editor.h"
#include "halley/audio/audio_sub_object.h"

namespace Halley {
	class AudioProperties;
	class AudioObjectEditorTreeList;

	class AudioObjectEditor : public AssetEditor {
    public:
        AudioObjectEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);

        void onResourceLoaded() override;
        void refreshAssets() override;

		bool isModified() override;
		void save() override;
        void markModified(bool refreshList);

        void onMakeUI() override;

        bool canDragItem(const String& itemId) const;
        bool canParentItemTo(const String& itemId, const String& parentId) const;

		const AudioProperties& getAudioProperties() const;

    protected:
        void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const Path& assetPath, const String& assetId, AssetType assetType) override;

	private:
        struct TreeData {
            String parent;
	        IAudioObject* object = nullptr;
            std::optional<String> subCase = {};
            std::optional<String> clip;
        };

        ProjectWindow& projectWindow;
		bool modified = false;
        bool needFullRefresh = false;
        std::shared_ptr<AudioObject> audioObject;
        std::shared_ptr<AudioObjectEditorTreeList> hierarchy;
        HashMap<String, TreeData> treeData;

        IAudioObject* currentObject = nullptr;
        std::shared_ptr<UIWidget> currentObjectEditor;

        void doLoadUI();
        void populateTreeView(const String& parentId, AudioSubObjectHandle& subObject);
        void populateTreeData();
        void populateTreeData(const String& parentId, AudioSubObjectHandle& subObject);

        void onSelectionChange(const String& id);
        void addObject();
        void addObject(AudioSubObjectType type);
        void addClip();
        void addClip(const String& assetId);
        void removeCurrentSelection();

		void moveItem(const String& itemId, const String& parentId, const String& oldParentId, int childIdx, int oldChildIdx);
        void moveObject(const String& itemId, const String& parentId, const String& oldParentId, int childIdx, int oldChildIdx);
        void moveClip(const String& itemId, const String& parentId, const String& oldParentId, int childIdx, int oldChildIdx);

        void setCurrentObject(IAudioObject* object);
        void doSetCurrentObject();
        void setCurrentObjectEditor(std::shared_ptr<UIWidget> widget);

		Sprite makeIcon(AudioSubObjectType type) const;
	};

    class AudioObjectEditorTreeList : public UITreeList {
    public:
        AudioObjectEditorTreeList(String id, UIStyle style);

    	void setParent(AudioObjectEditor& parent);

    protected:
        bool canParentItemTo(const String& itemId, const String& parentId) const override;
        bool canDragItemId(const String& itemId) const override;

    private:
        AudioObjectEditor* parent = nullptr;
    };

	class ChooseAudioSubObject : public ChooseAssetWindow {
	public:
        ChooseAudioSubObject(UIFactory& factory, Callback callback);

	private:
		void sortItems(Vector<std::pair<String, String>>& items) override;
	};
}
