#pragma once

#include "vector2.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class LineSegment;
	class Circle;
	class Polygon;

	class Ray
	{
	public:
		struct RayCastResult {
			float distance;
			Vector2f normal;
			Vector2f pos;

			RayCastResult() = default;
			RayCastResult(float distance, Vector2f normal, Vector2f pos)
				: distance(distance), normal(normal), pos(pos)
			{}
		};

		Vector2f p;
		Vector2f dir;

		Ray();
		Ray(Vector2f start, Vector2f dir);

		std::optional<RayCastResult> castCircle(Vector2f centre, float radius) const;
		std::optional<RayCastResult> castCircle(const Circle& circle) const;
		std::optional<RayCastResult> castLineSegment(Vector2f a, Vector2f b) const;
		std::optional<RayCastResult> castLineSegment(const LineSegment& lineSegment) const;
		std::optional<RayCastResult> castPolygon(const Polygon& polygon) const;
	};
}