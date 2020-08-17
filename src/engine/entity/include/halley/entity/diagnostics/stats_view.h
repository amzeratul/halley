#pragma once

#include "halley/core/graphics/text/text_renderer.h"
#include "halley/time/halleytime.h"
#include <cstdint>

namespace Halley {
    class CoreAPI;
    class Resources;
    class RenderContext;
    class World;

	class StatsView {
    public:
        StatsView(Resources& resources, CoreAPI& coreApi);
        virtual ~StatsView() = default;

        void draw(RenderContext& context);

		virtual void update();
        virtual void paint(Painter& painter) = 0;

		void setWorld(const World* world);

    protected:
        Resources& resources;
        CoreAPI& coreAPI;
        const World* world = nullptr;

        String formatTime(int64_t ns) const;
    };
}