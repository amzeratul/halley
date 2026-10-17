#pragma once
#include "halley/maths/rect.h"

namespace Halley {
    class Image;
    class Sprite;
    class Polygon;

    class ImagePolygonExtractor {
    public:
        static Polygon extractPolygon(const Sprite& sprite);
        static Polygon extractPolygon(const Image& image, Rect4i bounds);
    };
}
