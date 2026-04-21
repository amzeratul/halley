#pragma once
#include "sprite.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class Painter;
	class Resources;

	class IPainter {
    public:
        virtual ~IPainter() = default;

        virtual void update(Time t, Resources& resources) {}
        virtual void copyPrevious(const IPainter& prev) {}

        virtual void startFrame(bool multithreaded) {}
        virtual void startRender(bool waitForSpriteLoad, bool depthQueriesEnabled, std::optional<uint16_t> worldPartition) {}
        virtual void endRender() {}

        void draw(SpriteMaskBase mask, Painter& painter)
        {
	        drawOpaque(mask, painter);
	        drawTransparent(mask, painter);
        }

        virtual void drawOpaque(SpriteMaskBase mask, Painter& painter) {}
        virtual void drawTransparent(SpriteMaskBase mask, Painter& painter) {}
        virtual void clear() {}
    };

}
