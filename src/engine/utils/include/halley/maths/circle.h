#pragma once

#include "vector2.h"
#include <vector>

namespace Halley {
    class Circle {
    public:
        Circle() = default;
        Circle(Vector2f centre, float radius)
            : centre(centre)
            , radius(radius)
        {}

        float getRadius() const { return radius; }
        Vector2f getCentre() const { return centre; }

    	bool contains(Vector2f point) const;
        bool overlaps(const Circle& circle) const;

    	[[nodiscard]] Circle expand(float radius) const;

        static Circle getSpanningCircle(const std::vector<Vector2f>& points);

    private:
        Vector2f centre;
        float radius;
    };
}
