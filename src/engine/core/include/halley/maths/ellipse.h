#pragma once

#include "vector2.h"
#include "halley/data_structures/vector.h"
#include "rect.h"

namespace Halley {
	class LineSegment;

	class Ellipse {
    public:
        Ellipse() = default;
        Ellipse(Vector2f centre, Vector2f radii)
            : centre(centre)
            , radii(radii)
        {}

        Vector2f getRadii() const { return radii; }
        Vector2f getCentre() const { return centre; }

    	[[nodiscard]] bool contains(Vector2f point) const;
        [[nodiscard]] Rect4f getAABB() const;

    private:
        Vector2f centre;
        Vector2f radii;
    };

}
