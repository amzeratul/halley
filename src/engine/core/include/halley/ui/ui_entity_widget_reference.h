#pragma once
#include "ui_widget.h"

namespace Halley {
    class UIEntityWidgetReference {
    public:
        String id;
        Vector2f offset;
        Vector2f alignment;
        std::shared_ptr<UIWidget> widget;

        bool alive = true;
        bool screenBound = false;
        bool round = false;
        Vector4f screenBorders;

        UIEntityWidgetReference() = default;
        UIEntityWidgetReference(String id, Vector2f offset, Vector2f alignment, std::shared_ptr<UIWidget> widget);
    };
}
