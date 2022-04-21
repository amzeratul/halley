#pragma once

#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
    class AudioRootEditor : public UIWidget {
    public:
        AudioRootEditor(UIFactory& factory, AudioObject& object);

    private:
        UIFactory& factory;
        AudioObject& object;
    };
}
