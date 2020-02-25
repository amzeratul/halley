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

#include "halley/maths/aabb.h"

using namespace Halley;

AABB::AABB()
{
}

AABB::AABB(Vector2f _p1, Vector2f _p2)
	: p1(_p1), p2(_p2)
{
}

void AABB::set(Vector2f _p1, Vector2f _p2)
{
	p1 = _p1;
	p2 = _p2;
}

Rect4f AABB::toRect4f() const
{
	return Rect4f(p1, p2);
}

AABB AABB::getSpanningBox(const std::vector<Vector2f>& points)
{
	float x1 = std::numeric_limits<float>::max();
	float y1 = std::numeric_limits<float>::max();
	float x2 = std::numeric_limits<float>::lowest();
	float y2 = std::numeric_limits<float>::lowest();

	const size_t len = points.size();
	for (size_t i = 0; i < len; i++) {
		Vector2f v = points[i];
		if (v.x < x1) {
			x1 = v.x;
		}
		if (v.x > x2) {
			x2 = v.x;
		}
		if (v.y < y1) {
			y1 = v.y;
		}
		if (v.y > y2) {
			y2 = v.y;
		}
	}
	
	return AABB(Vector2f(x1, y1), Vector2f(x2, y2));
}

bool AABB::overlaps(const AABB& p, Vector2f delta) const
{
	if (p.p1.x > p2.x + delta.x) return false;
	if (p1.x > p.p2.x + delta.x) return false;
	if (p.p1.y > p2.y + delta.y) return false;
	if (p1.y > p.p2.y + delta.y) return false;
	return true;
}

bool AABB::isPointInside(Vector2f p) const
{
	return (p.x >= p1.x && p.x <= p2.x && p.y >= p1.y && p.y <= p2.y);
}
