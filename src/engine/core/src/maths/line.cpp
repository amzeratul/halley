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
#include "halley/maths/rect.h"
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

std::optional<LineSegment> LineSegment::clip(const Rect4f& rect) const
{
	const auto hRange = rect.getHorizontal();
	const auto vRange = rect.getVertical();

	const auto orig = *this;

	if (!hRange.overlaps(getHorizontal()) || !vRange.overlaps(getVertical())) {
		// No overlap
		return {};
	}

	const bool orderHorizontal = a.x < b.x;
	auto result = *this;

	if (!hRange.contains(result.a.x)) {
		// Clip horizontal A
		result.a = *intersection(Line(orderHorizontal ? rect.getTopLeft() : rect.getTopRight(), Vector2f(0, 1)));
		assert(orig.getDistance(result.a) < 0.1f);
	}
	if (!hRange.contains(result.b.x)) {
		// Clip horizontal B
		result.b = *intersection(Line(orderHorizontal ? rect.getTopRight() : rect.getTopLeft(), Vector2f(0, 1)));
		assert(orig.getDistance(result.b) < 0.1f);
	}

	if (!vRange.overlaps(result.getVertical())) {
		// No overlap
		return {};
	}

	const bool orderVertical = result.a.y < result.b.y;

	if (!vRange.contains(result.a.y)) {
		// Clip vertical A
		result.a = *result.intersection(Line(orderVertical ? rect.getTopLeft() : rect.getBottomLeft(), Vector2f(1, 0)));
		assert(orig.getDistance(result.a) < 0.1f);
	}
	if (!vRange.contains(result.b.y)) {
		// Clip vertical B
		result.b = *result.intersection(Line(orderVertical ? rect.getBottomLeft() : rect.getTopLeft(), Vector2f(1, 0)));
		assert(orig.getDistance(result.b) < 0.1f);
	}

	return result;
}
