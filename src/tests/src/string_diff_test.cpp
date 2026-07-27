#include "halley/text/string_diff.h"

#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

TEST(HalleyStringDiffTest, Test)
{
	std::string_view a = "Ah, the oscilloscope has detected a sine wave!";
	std::string_view b = "We have detected a most unusual sine wave!";

	auto diff = StringDiff::makeWordDiff(a, b);

	for (auto& d: diff) {
		Logger::logInfo("[" + toString(d.type) + "] " + d.str);
	}

	EXPECT_EQ(1, 1);
}
