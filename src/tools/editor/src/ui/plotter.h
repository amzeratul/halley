#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
    class Plotter : public UIWidget {
    public:
        Plotter(UIFactory& factory);

        void onMakeUI() override;

    private:
        UIFactory& factory;
    };
}
