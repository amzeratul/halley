#pragma once

#include <halley/ui/ui_widget.h>

namespace Halley {
    class UIFactory;

    class PopupWindow : public UIWidget {
    public:
        PopupWindow(String id);

        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
        void releaseMouse(Vector2f mousePos, int button) override;
        void onMouseOver(Vector2f mousePos) override;
        bool isFocusLocked() const override;

    private:
        bool dragging = false;
        Vector2f startDragPos;
    };
}
