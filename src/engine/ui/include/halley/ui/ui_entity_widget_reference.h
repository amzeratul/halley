#pragma once
#include "ui_widget.h"

namespace Halley {
    class UIEntityWidgetReference {
    public:
        String id;
        Vector2f offset;
        Vector2f alignment;
        std::shared_ptr<UIWidget> widget;

        UIEntityWidgetReference() = default;
        UIEntityWidgetReference(String id, Vector2f offset, Vector2f alignment, std::shared_ptr<UIWidget> widget);
    };
}
