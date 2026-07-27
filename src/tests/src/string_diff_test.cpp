#include "halley/text/string_diff.h"

#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

TEST(HalleyStringDiffTest, Test)
{
	std::string_view a = "Hello world, how are all of you?";
	std::string_view b = "Hello wonderful world, how are you!?";

	auto diff = StringDiff::makeWordDiff(a, b);

	for (auto& d: diff) {
		Logger::logInfo("[" + toString(d.type) + "] " + d.str);
	}

	EXPECT_EQ(1, 1);
}
