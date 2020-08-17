#pragma once

#include "halley/core/graphics/text/text_renderer.h"
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

        virtual void draw(RenderContext& context) = 0;

		void setWorld(const World* world);

    protected:
        Resources& resources;
        const CoreAPI& coreAPI;
        const World* world = nullptr;

        String formatTime(int64_t ns) const;
    };
}