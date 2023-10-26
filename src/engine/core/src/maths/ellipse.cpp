#include "halley/maths/ellipse.h"

#include "halley/maths/line.h"
#include "halley/maths/random.h"
#include "halley/maths/triangle.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

bool Ellipse::contains(Vector2f point) const
{
	const auto p = point - centre;
	const auto a = radii.x;
	const auto b = radii.y;
	return (p.x * p.x) / (a * a) + (p.y * p.y) / (b * b) <= 1.0f;
}

Rect4f Ellipse::getAABB() const
{
	return Rect4f(centre - radii, centre + radii);
}
