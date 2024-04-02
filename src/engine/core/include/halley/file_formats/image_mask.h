#pragma once
#include "halley/maths/rect.h"
#include "halley/maths/vector2.h"

namespace Halley {
	class Image;

	class ImageMask {
    public:
        ImageMask() = default;
        ImageMask(Vector2i size, bool clear);

		static ImageMask fromAlpha(const Image& image);

        bool isSet(Vector2i pos) const;
        void set(Vector2i pos, bool value);

        Vector2i getSize() const;
        Rect4i getRect() const;

	private:
        Vector2i size;
        Vector<uint8_t> values;
    };
}
