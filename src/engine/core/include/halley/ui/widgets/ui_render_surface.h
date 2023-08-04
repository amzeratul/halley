#pragma once

#include "../ui_widget.h"

namespace Halley {
    class UIRenderSurface : public UIWidget {
    public:
        UIRenderSurface(String id, Vector2f minSize = Vector2f(), std::optional<UISizer> sizer = std::nullopt);

        void draw(UIPainter& painter) const override;
        void drawChildren(UIPainter& painter) const override;
        void render(RenderContext& rc) const override;
    };
}
