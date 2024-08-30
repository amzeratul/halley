#pragma once

#include "vector2.h"
#include "halley/data_structures/vector.h"
#include "rect.h"

namespace Halley {
	class LineSegment;

	class Circle {
    public:
        Circle() = default;
        Circle(Vector2f centre, float radius)
            : centre(centre)
            , radius(radius)
        {}
        Circle(const LineSegment& segment);

        float getRadius() const { return radius; }
        Vector2f getCentre() const { return centre; }

    	[[nodiscard]] bool contains(Vector2f point) const;
        [[nodiscard]] float getDistanceTo(Vector2f point) const;
        [[nodiscard]] float getDistanceTo(const Circle& circle) const;
        [[nodiscard]] bool overlaps(const Circle& circle) const;

    	[[nodiscard]] Circle expand(float radius) const;
        [[nodiscard]] Rect4f getAABB() const;
        [[nodiscard]] Vector2f project(Vector2f point) const;

        static Circle getSpanningCircle(const Vector<Vector2f>& points);
        static Circle getSpanningCircle2(Vector<Vector2f> points);
        static Circle getInscribedCircle(Rect4f rect);

        static std::optional<Circle> getCircleTangentToAngle(Vector2f A, Vector2f B, Vector2f C, float radius); // corner ABC, where B is the angle

    private:
        Vector2f centre;
        float radius;

        static Circle getSpanningCircleTrivial(gsl::span<Vector2f> ps);
        static Circle msw(gsl::span<Vector2f> ps, gsl::span<Vector2f, 3> rs);
        static Vector2f& mswNonBase(Vector2f p, gsl::span<Vector2f, 3> rs);
    };

	static_assert(std::is_trivially_copyable_v<Circle>);
}
