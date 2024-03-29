#pragma once

#include "../ui_widget.h"
#include "halley/game/frame_data_sync.h"
#include "halley/graphics/render_target/render_surface.h"
#include "halley/graphics/sprite/sprite_painter.h"

namespace Halley {
    class UIRenderSurface : public UIWidget {
    public:
        UIRenderSurface(String id, Vector2f minSize, std::optional<UISizer> sizer, const HalleyAPI& api, Resources& resources, const String& materialName, RenderSurfaceOptions options);

        void draw(UIPainter& painter) const override;
        void drawChildren(UIPainter& painter) const override;
        void render(RenderContext& rc) const override;
        void renderChildren(RenderContext& rc) const override;

        void setColour(Colour4f col);
        void setScale(Vector2f scale);

        Vector2f getLayoutMinimumSize(bool force) const override;
        Vector2f getLayoutSize(Vector2f size) const override;
        void onPreNotifySetRect(IUIElementListener& listener) override;

        std::optional<Vector2f> transformToChildSpace(Vector2f pos) const override;
        
    private:
        std::unique_ptr<RenderSurface> renderSurface;

        Colour4f colour;
        Vector2f scale;
        mutable Vector2f innerSize;

        struct RenderParams {
            int mask;
            Vector2f pos;
            Vector2f size;
            Vector2f scale;
            Vector4f border;
		    Colour4f colour;
			std::unique_ptr<SpritePainter> spritePainter;
        };

        mutable FrameDataSync<RenderParams> paramsSync;
        mutable std::optional<RenderParams> renderParams;
        mutable Vector2f origScale;

        void drawOnPainter(Painter& painter) const;
    };
}
