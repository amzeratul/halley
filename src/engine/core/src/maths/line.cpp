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

void Halley::Line::doLine(Vector2i p0, Vector2i p1, std::function<void(Vector2i)> callback)
{
	// Using Bresenham's line algorithm
	// http://en.wikipedia.org/wiki/Bresenham's_line_algorithm

	const int x0 = p0.x;
	const int y0 = p0.y;
	const int x1 = p1.x;
	const int y1 = p1.y;

	int x = x0;
	int y = y0;
	//callback(Vector2i(x, y));

	const int dx = abs(x1-x0);
	const int dy = abs(y1-y0);
	const int sx = (x0 < x1) ? 1 : -1;
	const int sy = (y0 < y1) ? 1 : -1;
	int err = dx-dy;

	while (true) {
		callback(Vector2i(x, y));

		// End of line
		if (x == x1 && y == y1)	return;

		int e2 = 2*err;
		if (e2 > -dy) {
			err -= dy;
			x += sx;
		}
		if (e2 < dx) {
			err += dx;
			y += sy;
		}
	}
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
