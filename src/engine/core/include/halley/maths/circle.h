#pragma once

#include "line.h"
#include "vector2.h"
#include "halley/data_structures/vector.h"
#include "rect.h"

namespace Halley {
	class Circle {
    public:
        constexpr Circle() = default;
        
		constexpr Circle(Vector2f centre, float radius)
            : centre(centre)
            , radius(radius)
        {}
        
		constexpr Circle(const LineSegment& segment)
        {
			centre = 0.5f * (segment.a + segment.b);
			radius = 0.5f * (segment.a - segment.b).length();
        }

        [[nodiscard]] constexpr float getRadius() const { return radius; }
        [[nodiscard]] constexpr Vector2f getCentre() const { return centre; }

    	[[nodiscard]] constexpr bool contains(Vector2f point) const
		{
			return (point - centre).squaredLength() <= radius * radius;
		}

        [[nodiscard]] constexpr float getDistanceTo(Vector2f point) const
        {
			return std::max((centre - point).length() - radius, 0.0f);
		}

		[[nodiscard]] constexpr float getDistanceTo(const Circle& circle) const
        {
			return std::max((centre - circle.centre).length() - radius - circle.radius, 0.0f);
		}

		[[nodiscard]] constexpr bool overlaps(const Circle& circle) const
        {
			const float r = radius + circle.radius;
			return (circle.centre - centre).squaredLength() <= r * r;
		}

		[[nodiscard]] constexpr bool isDistanceAtMost(Vector2f point, float maxDist) const
		{
			const float r = maxDist + radius;
			const float r2 = r * r;
			return (centre - point).squaredLength() <= r2;
		}

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
