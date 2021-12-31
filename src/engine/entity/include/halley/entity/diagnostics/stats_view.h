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
		
        virtual void draw(RenderContext& context);

		virtual void update() = 0;

	protected:
        virtual void paint(Painter& painter) = 0;
	};

	class StatsView : public ScreenOverlay {
    public:
        StatsView(Resources& resources, const HalleyAPI& api);
        virtual ~StatsView() = default;

		void update() override;
		void draw(RenderContext& context) override;

		void setActive(bool active);
		bool isActive() const;

		void setWorld(const World* world);

    protected:
        Resources& resources;
        const HalleyAPI& api;
        const World* world = nullptr;
		bool active = true;

        String formatTime(int64_t ns) const;
    };
}
