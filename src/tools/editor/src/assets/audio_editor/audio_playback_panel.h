#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
    class AudioPlaybackPanel : public UIWidget {
    public:
        AudioPlaybackPanel(UIFactory& factory, const HalleyAPI& api, Project& project);

        void onMakeUI() override;
        void update(Time t, bool moved) override;
        void onActiveChanged(bool active) override;

        void onObjectModified();
        void setAudioObject(std::shared_ptr<const AudioObject> object);
        void setAudioEvent(std::shared_ptr<const AudioEvent> event);

    private:
        UIFactory& factory;
    	const HalleyAPI& api;
        Project& project;

        std::shared_ptr<const AudioObject> object;
        std::shared_ptr<const AudioEvent> event;
    	std::shared_ptr<AudioObject> playbackObject;
        std::shared_ptr<UIButton> playButton;

        bool needsIconUpdate = false;
        bool needsObjectUpdate = false;
        bool needsLoadingVariables = false;
        Time updateCooldown = 0;

    	AudioEmitterHandle emitter;
        AudioHandle audioHandle;
        AudioPosition audioPosition;

        enum class VariableType {
	        Variable,
            Switch
        };

        Vector<std::pair<VariableType, String>> variables;
        HashMap<String, float> curVars;
        HashMap<String, String> curSwitches;

        void onPlay();
        void play();
        void pause();
        bool isPlaying() const;
        bool isReadyToPlay() const;
        void updatePlaybackObject();

        Vector<std::pair<VariableType, String>> getCurrentVariableList() const;
        void loadVariables();
        void populateVariables();
        std::shared_ptr<UIWidget> makeVariableControl(VariableType type, const String& id);
        void applyVariablesToEmitter();

        void setVariable(const String& id, float value);
        void setSwitch(const String& id, const String& value);
    };
}
