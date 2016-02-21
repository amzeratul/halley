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

#include "aabb.h"

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
