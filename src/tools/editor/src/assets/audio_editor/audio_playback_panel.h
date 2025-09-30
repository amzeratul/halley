#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
    class AudioPlaybackPanel : public UIWidget {
    public:
        AudioPlaybackPanel(UIFactory& factory, const HalleyAPI& api);

        void onMakeUI() override;
        void update(Time t, bool moved) override;

        void setAudioObject(std::shared_ptr<const AudioObject> object);
        void setAudioEvent(std::shared_ptr<const AudioEvent> event);

    private:
        UIFactory& factory;
    	const HalleyAPI& api;

        std::shared_ptr<const AudioObject> object;
        std::shared_ptr<const AudioEvent> event;

        bool needsIconUpdate = false;

    	AudioEmitterHandle emitter;
        AudioHandle audioHandle;

        void onPlay();
        void play();
        void pause();
        bool isPlaying() const;
    };
}
