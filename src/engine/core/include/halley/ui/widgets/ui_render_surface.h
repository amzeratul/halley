#pragma once

#include "../ui_widget.h"
#include "halley/graphics/render_target/render_surface.h"
#include "halley/graphics/sprite/sprite_painter.h"

namespace Halley {
    class UIRenderSurface : public UIWidget {
    public:
        UIRenderSurface(String id, Vector2f minSize, std::optional<UISizer> sizer, const HalleyAPI& api, Resources& resources, const String& materialName, RenderSurfaceOptions options);

        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;
        void drawChildren(UIPainter& painter) const override;
        void render(RenderContext& rc) const override;

    private:
        std::unique_ptr<SpritePainter> spritePainter;
        std::unique_ptr<RenderSurface> renderSurface;

        struct RenderParams {
            int mask;
            Rect4f bounds;
        };
        mutable RenderParams params;

        void drawOnPainter(Painter& painter) const;
    };
}
