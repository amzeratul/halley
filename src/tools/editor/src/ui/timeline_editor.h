#pragma once

#include "prec.h"

namespace Halley
{
    class TimelineEditor : public UIWidget {
    public:
        TimelineEditor(String id, UIFactory& factory);

        void onMakeUI() override;
    };
}
