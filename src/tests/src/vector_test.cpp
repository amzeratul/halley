#include <gtest/gtest.h>
#include <halley.hpp>

#include "halley/data_structures/vector_size32.h"
using namespace Halley;

TEST(VectorSize32, Construction)
{
	VectorSize32<int> a;

	for (int i = 0; i < 100; ++i) {
		a.push_back(i);
	}

	for (int i = 0; i < 100; ++i) {
		EXPECT_EQ(i, a[i]);
	}
}
