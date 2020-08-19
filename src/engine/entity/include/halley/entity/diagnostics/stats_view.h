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

	class StatsView {
    public:
        StatsView(Resources& resources, const HalleyAPI& api);
        virtual ~StatsView() = default;

        void draw(RenderContext& context);

		virtual void update();
        virtual void paint(Painter& painter) = 0;

		void setWorld(const World* world);

    protected:
        Resources& resources;
        const HalleyAPI& api;
        const World* world = nullptr;
        StopwatchRollingAveraging timer;
        bool drawing = false;

        String formatTime(int64_t ns) const;
    };
}
