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

#include "halley/maths/line.h"

#include "halley/maths/circle.h"
#include "halley/maths/ray.h"

using namespace Halley;


Vector<Vector2i> Line::generateLine(Vector2i p0, Vector2i p1)
{
	Vector<Vector2i> result;
	result.reserve(std::max(std::abs(p0.x - p1.x), std::abs(p0.y - p1.y)) + 1);

	doLine(p0, p1, [&] (Vector2i p)
	{
		result.push_back(p);
	});

	return result;
}

std::optional<LineSegment> LineSegment::clip(const Circle& circle) const {
	const bool aInside = circle.contains(a);
	const bool bInside = circle.contains(b);
	if (aInside && bInside) {
		// Fully inside, no clip
		return *this;
	} else if (aInside) {
		// Clip b
		LineSegment result = *this;
		result.b = Ray(b, (a - b).normalized()).castCircle(circle)->pos;
		return result;
	} else if (bInside) {
		// Clip a
		LineSegment result = *this;
		result.a = Ray(a, (b - a).normalized()).castCircle(circle)->pos;
		return result;
	} else if (getDistance(circle.getCentre()) <= circle.getRadius()) {
		// Clip both ends
		LineSegment result;
		result.a = Ray(a, (b - a).normalized()).castCircle(circle)->pos;
		result.b = Ray(b, (a - b).normalized()).castCircle(circle)->pos;
		return result;
	} else {
		// No overlap
		return std::nullopt;
	}
}
