#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;

	class AudioFadeEditor : public UIWidget {
    public:
        AudioFadeEditor(UIFactory& factory, AudioFade& fade, std::function<void()> onModifiedCallback);

        void onMakeUI() override;

    private:
        UIFactory& factory;
        AudioFade& fade;
        std::function<void()> onModifiedCallback;
    };
}
