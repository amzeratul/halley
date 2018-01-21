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

using namespace Halley;


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

Vector2f Line::closestPointInSegment(Vector2f start, Vector2f end, Vector2f point)
{
	float len = (end - start).length();
	Vector2f n = (end - start) / len;
	Vector2f p = point - start;
	float t = p.dot(n);
	if (t < 0) {
		return start;
	} else if (t >= len) {
		return end;
	} else {
		return start + n * t;
	}
}
