#pragma once

#include "vector2.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class Polygon;

	class Ray
	{
	public:
		Vector2f p;
		Vector2f dir;

		Ray();
		Ray(Vector2f start, Vector2f dir);

		std::optional<std::pair<float, Vector2f>> castCircle(Vector2f centre, float radius) const;
		std::optional<std::pair<float, Vector2f>> castLineSegment(Vector2f a, Vector2f b) const;
		std::optional<std::pair<float, Vector2f>> castPolygon(const Polygon& polygon) const;
	};
}