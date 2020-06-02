/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "vector2.h"
#include <optional>

namespace Halley {
	class Line {
	public:
		constexpr Line() = default;
		constexpr Line(Vector2f origin, Vector2f dir)
			: origin(origin)
			, dir(dir.normalized())
		{}
		
		Vector2f origin;
		Vector2f dir;

		float getDistance(Vector2f point) const
		{
			return (point - origin).neutralize(dir).length();
		}

		Vector2f getClosestPoint(Vector2f point) const
		{
			return (point - origin).projection(dir) + origin;
		}

		std::optional<Vector2f> intersection(const Line& other) const
		{
			const auto& a = origin;
			const auto& b = dir;
			const auto& c = other.origin;
			const auto& d = other.dir;
			const float divisor = b.x * d.y - b.y * d.x;
			if (std::abs(divisor) < 0.000001f) {
				// Parallel lines
				return {};
			}
			const float t = (d.x * (a.y - c.y) + d.y * (c.x - a.x)) / divisor;
			return a + t * b;
		}

		static std::vector<Vector2i> generateLine(Vector2i p0, Vector2i p1);
		static void doLine(Vector2i p0, Vector2i p1, std::function<void(Vector2i)> callback);
	};

	class LineSegment {
	public:
		constexpr LineSegment() = default;
		constexpr LineSegment(Vector2f a, Vector2f b)
			: a(a), b(b)
		{}

		Vector2f a;
		Vector2f b;

		constexpr Vector2f getClosestPoint(Vector2f point) const
		{
			const float len = (b - a).length();
			const Vector2f dir = (b - a) * (1.0f / len);
			const float x = (point - a).dot(dir); // position along the A-B segment
			return a + dir * clamp(x, 0.0f, len);
		}
	};
}
