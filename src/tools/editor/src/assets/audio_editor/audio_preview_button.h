#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
    class AudioPreviewButton : public UIWidget {
    public:
        AudioPreviewButton(UIFactory& factory, Resources& resources, const HalleyAPI& api, AssetType type, String assetId);

        void onMakeUI() override;

        void update(Time t, bool moved) override;

    private:
        UIFactory& factory;
    	Resources& resources;
        const HalleyAPI& api;
    	AssetType type;
    	String assetId;

        bool buttonShowsPlaying = false;

    	AudioEmitterHandle emitter;
        AudioHandle audioHandle;

        void onPlay();
        void play();
        void stop();
        bool isPlaying();
    };

    class AudioPreviewUISharedData : public IUISharedData {
    public:
        UIWidget* lastActive = nullptr;
    };
}
