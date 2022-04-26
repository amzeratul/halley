#pragma once
#include "../asset_editor.h"

namespace Halley {
	class AudioProperties;

	class AudioEventEditor : public AssetEditor {
    public:
        AudioEventEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);

        void reload() override;
        void refreshAssets() override;
		void onMakeUI() override;
		
		void save() override;
		bool isModified() override;

		Resources& getGameResources() const;
        void markModified();

        void addAction();
        void addAction(AudioEventActionType type);
        void deleteAction(const IAudioEventAction& action, const String& uiId);

		const AudioProperties& getAudioProperties() const;

    protected:
        void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const String& assetId) override;

	private:
        std::shared_ptr<AudioEvent> audioEvent;
        std::shared_ptr<UIList> actionList;
        int actionId = 0;
        bool modified = false;

        void addActionUI(IAudioEventAction& action);
        void doLoadUI();
	};

	class AudioEventEditorAction : public UIWidget {
	public:
        AudioEventEditorAction(UIFactory& factory, AudioEventEditor& editor, IAudioEventAction& action, int id);
        void onMakeUI() override;
	
	private:
        UIFactory& factory;
        AudioEventEditor& editor;
        IAudioEventAction& action;
		
        void makeObjectAction(AudioEventActionObject& action);
        void makePlayAction(AudioEventActionPlay& action);
        void makeStopAction(AudioEventActionStop& action);
        void makePauseAction(AudioEventActionPause& action);
        void makeResumeAction(AudioEventActionResume& action);
        void makeSetVolumeAction(AudioEventActionSetVolume& action);
        void makeSetSwitchAction(AudioEventActionSetSwitch& action);
        void makeSetVariableAction(AudioEventActionSetVariable& action);
	};

	class ChooseAudioEventAction : public ChooseAssetWindow {
	public:
        ChooseAudioEventAction(UIFactory& factory, Callback callback);

	private:
		void sortItems(Vector<std::pair<String, String>>& items) override;
	};
}
