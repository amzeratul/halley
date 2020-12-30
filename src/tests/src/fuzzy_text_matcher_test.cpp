#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

TEST(HalleyFuzzyTextMatcher, Match)
{
	FuzzyTextMatcher matcher(false, 100);
	matcher.addString("image/environment/nature/plants/grass8.png");
	
    EXPECT_EQ(matcher.match("grass").size(), 1);
	EXPECT_EQ(matcher.match("grasss").size(), 1);
}
