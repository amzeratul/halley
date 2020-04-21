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

namespace Halley {
	class Line {
	public:
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
