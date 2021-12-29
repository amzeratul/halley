#pragma once

#include "halley/core/graphics/text/text_renderer.h"
#include "halley/time/halleytime.h"
#include <cstdint>

#include "halley/time/stopwatch.h"

namespace Halley {
	class HalleyAPI;
    class Resources;
    class RenderContext;
    class World;

	class ScreenOverlay {
	public:
		virtual ~ScreenOverlay() = default;
		
        virtual void draw(RenderContext& context, bool fullDraw);

		virtual void update() = 0;

	protected:
        virtual void paint(Painter& painter) = 0;
		virtual void paintHidden(Painter& painter);
	};

	class StatsView : public ScreenOverlay {
    public:
        StatsView(Resources& resources, const HalleyAPI& api);
        virtual ~StatsView() = default;

		void update() override;
		void draw(RenderContext& context, bool fullDraw) override;

		void setWorld(const World* world);

    protected:
        Resources& resources;
        const HalleyAPI& api;
        const World* world = nullptr;

        String formatTime(int64_t ns) const;
    };
}
