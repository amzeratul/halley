#pragma once
#include "asset_editor.h"

namespace Halley {
	class AudioEventEditor : public AssetEditor {
    public:
        AudioEventEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);

        void reload() override;
        void refreshAssets() override;
		void onMakeUI() override;
	
    protected:
        void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const String& assetId) override;

	private:
        std::shared_ptr<AudioEvent> audioEvent;
        std::shared_ptr<UIList> actionList;
        int actionId = 0;

        void doLoadUI();
	};

	class AudioEventEditorAction : public UIWidget {
	public:
        AudioEventEditorAction(UIFactory& factory, IAudioEventAction& action, int id);
        void onMakeUI() override;
	
	private:
        UIFactory& factory;
        IAudioEventAction& action;
	};
}
