#include "halley/maths/circle.h"
using namespace Halley;

bool Circle::contains(Vector2f point) const
{
	return (point - centre).squaredLength() <= radius * radius;
}

Circle Circle::getSpanningCircle(const std::vector<Vector2f>& points)
{
	// TODO: should use Matousek, Sharir, Welzl's algorithm (https://en.wikipedia.org/wiki/Smallest-circle_problem#Matou%C5%A1ek,_Sharir,_Welzl's_algorithm)

	Vector2f centre;
	for (auto& p: points) {
		centre += p;
	}
	centre /= float(points.size());

	float radius2 = 0;
	for (auto& p: points) {
		const auto d = (p - centre).squaredLength();
		if (d > radius2) {
			radius2 = d;
		}
	}

	return Circle(centre, std::sqrtf(radius2));
}
