#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

TEST(HalleyPolygon, Subtraction)
{
    std::vector<Vector2f> vsA = { { 67, 202 }, { 244, 65 }, { 421, 99 }, { 424, 305 }, { 217, 447 }, { 112, 374 } };
    std::vector<Vector2f> vsB = { { 67, 114 }, { 325, 500 }, { 426, 422 }, { 297, 152 } };

    Polygon polyA(std::move(vsA));
    Polygon polyB(std::move(vsB));

	auto result = polyA.subtract(polyB);
}
