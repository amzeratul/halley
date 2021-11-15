#pragma once

namespace Halley {
    class UIWidgetEditor : public UIWidget {
    public:
        UIWidgetEditor(String id, UIFactory& factory);

    private:
        UIFactory& factory;
    };
}
