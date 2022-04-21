#pragma once

#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class AudioObjectEditor;

	class AudioRootEditor : public UIWidget {
    public:
        AudioRootEditor(UIFactory& factory, AudioObjectEditor& editor, AudioObject& object);

        void onMakeUI() override;

    private:
        UIFactory& factory;
        AudioObjectEditor& editor;
        AudioObject& object;
    };
}
