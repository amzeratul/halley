#include <gtest/gtest.h>
#include <halley.hpp>

#include "halley/data_structures/vector_size32.h"
using namespace Halley;

namespace {
	template <typename T>
	void testConstruction() {
		T a;

		for (int i = 0; i < 100; ++i) {
			a.push_back(i);
		}

		for (int i = 0; i < 100; ++i) {
			EXPECT_EQ(i, a[i]);
		}
	}
	
	template <typename T>
	void testErase() {
		T a;

		for (int i = 0; i < 10; ++i) {
			a.push_back(i);
		}

		a.erase(a.begin() + 5);

		EXPECT_EQ(a.size(), 9);
		EXPECT_EQ(a[4], 4);
		EXPECT_EQ(a[5], 6);
	}
}

TEST(StdVector, Construction)
{
	testConstruction<std::vector<int>>();
}

TEST(StdVector, Erase)
{
	testErase<std::vector<int>>();
}

TEST(VectorSize32, Construction)
{
	testConstruction<VectorSize32<int>>();
}

TEST(VectorSize32, Erase)
{
	testErase<VectorSize32<int>>();
}
