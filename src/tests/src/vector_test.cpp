#include <gtest/gtest.h>
#include <halley.hpp>
#include <vector>

#include "halley/data_structures/vector_size32.h"
using namespace Halley;

namespace {
	template <typename T>
	void testConstruction() {
		T a;

		for (int i = 0; i < 100; ++i) {
			a.push_back(i + 100);
		}

		for (int i = 0; i < 100; ++i) {
			EXPECT_EQ(i + 100, a[i]);
		}

		const auto b = T(5, 10);
		EXPECT_EQ(b.size(), 5);
		EXPECT_EQ(b.back(), 10);

		const auto c = T({ 1, 2, 3 });
		EXPECT_EQ(c.size(), 3);
		EXPECT_EQ(c[0], 1);
		EXPECT_EQ(c[1], 2);
		EXPECT_EQ(c[2], 3);
	}
	
	template <typename T>
	void testErase() {
		T a;

		EXPECT_TRUE(a.empty());

		for (int i = 0; i < 10; ++i) {
			a.push_back(i);
		}

		EXPECT_FALSE(a.empty());

		a.erase(a.begin() + 5);

		EXPECT_EQ(a.size(), 9);
		EXPECT_EQ(a[4], 4);
		EXPECT_EQ(a[5], 6);

		a.erase(a.begin(), a.begin() + 2);
		EXPECT_EQ(a.size(), 7);
		EXPECT_EQ(a[0], 2);
	}

	template <typename T>
	void testSelfMove(typename T::value_type v) {
		T a;

		a.push_back(v);
		for (int i = 0; i < 20; ++i) {
			a.push_back(std::move(a.back()));
		}

		EXPECT_EQ(a.front(), T::value_type());
		EXPECT_EQ(a.back(), v);
	}

	template <typename T>
	void testMoveOnly(typename T::value_type v) {
		T a;

		a.push_back(std::move(v));
		for (int i = 0; i < 20; ++i) {
			a.push_back(std::move(a.back()));
		}

		EXPECT_EQ(a.front(), T::value_type());
		EXPECT_NE(a.back(), T::value_type());
	}
	
	template <typename T>
	void testAssignment() {
		T a;

		for (int i = 0; i < 10; ++i) {
			a.push_back(i);
		}

		EXPECT_EQ(a[5], 5);

		T b;
		b = a;

		EXPECT_EQ(b[5], 5);

		for (int i = 0; i < 10; ++i) {
			EXPECT_EQ(a[i], b[i]);
		}
		
		T c;
		c = std::move(b);

		EXPECT_TRUE(b.empty());

		for (int i = 0; i < 10; ++i) {
			EXPECT_EQ(a[i], c[i]);
		}

		EXPECT_FALSE(a == b);
		EXPECT_TRUE(a == c);

		std::swap(b, c);
		EXPECT_TRUE(a == b);
		EXPECT_FALSE(a == c);
	}

	template <typename T>
	void testSBO(size_t expectedSBOSize)
	{
		if (!T::sbo_enabled()) {
			return;
		}

		constexpr size_t maxSBO = static_cast<size_t>(T::sbo_max_objects());
		EXPECT_EQ(maxSBO, expectedSBOSize);

		T a;

		for (size_t i = 0; i < maxSBO; ++i) {
			const auto val = i + 100;
			if constexpr (std::is_same_v<typename T::value_type, Halley::String>) {
				a.push_back(toString(val));
			} else {
				a.push_back(T::value_type(val));
			}
		}
		EXPECT_TRUE(a.sbo_active());

		T b = a;
		EXPECT_TRUE(b.sbo_active());
		b.push_back({});
		EXPECT_FALSE(b.sbo_active());
		b.pop_back();
		b.shrink_to_fit();
		EXPECT_TRUE(b.sbo_active());
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

TEST(StdVector, SelfMove)
{
	testSelfMove<std::vector<String>>("hello");
	testMoveOnly<std::vector<std::unique_ptr<int>>>(std::make_unique<int>(42));
}

TEST(StdVector, Assignment)
{
	testAssignment<std::vector<int>>();
}

TEST(VectorSize32, Construction)
{
	testConstruction<VectorSize32<int>>();
}

TEST(VectorSize32, Erase)
{
	testErase<VectorSize32<int>>();
}

TEST(VectorSize32, SelfMove)
{
	testSelfMove<VectorSize32<String>>("hello");
	testMoveOnly<VectorSize32<std::unique_ptr<int>>>(std::make_unique<int>(42));
}

TEST(VectorSize32, Assignment)
{
	testAssignment<VectorSize32<int>>();
}

TEST(VectorSize32, SBO)
{
	testSBO<VectorSize32<char>>(15);
	testSBO<VectorSize32<int>>(3);
	testSBO<VectorSize32<size_t>>(1);
	testSBO<VectorSize32<String>>(0);
}
