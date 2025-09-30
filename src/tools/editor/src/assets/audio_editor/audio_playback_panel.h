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

        bool needsIconUpdate = false;
        bool needsObjectUpdate = false;
        Time updateCooldown = 0;

    	AudioEmitterHandle emitter;
        AudioHandle audioHandle;

        void onPlay();
        void play();
        void pause();
        bool isPlaying() const;
        void updatePlaybackObject();
    };
}
