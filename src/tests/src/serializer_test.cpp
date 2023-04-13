#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

namespace {
	template <typename T>
	static T convertBackAndForth(T v)
	{
		static gsl::byte bytes[128];
		Serializer s(bytes, SerializerOptions(SerializerOptions::maxVersion));
		s << v;
		
		Deserializer ds(gsl::span<gsl::byte>(bytes, s.getPosition()), SerializerOptions(SerializerOptions::maxVersion));
		T result = 0;
		ds >> result;
		return result;
	}

	template <typename T>
	static void testRange(T minV, T maxV)
	{
		for (T i = minV; ; ++i) {
			EXPECT_EQ(i, convertBackAndForth(i));
			if (i == maxV) {
				return;
			}
		}
	}
}

TEST(Serializer, Int8Conversion)
{
	testRange(static_cast<int8_t>(-128), static_cast<int8_t>(127));
}

TEST(Serializer, IntConversion)
{
	testRange(static_cast<int>(-1000), static_cast<int>(1000));
}

TEST(Serializer, Int64Conversion)
{
	testRange(static_cast<int64_t>(-1000), static_cast<int64_t>(1000));
	testRange(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min() + 1000);
	testRange(std::numeric_limits<int64_t>::max() - 1000, std::numeric_limits<int64_t>::max());

	auto& rng = Random::getGlobal();
	for (int i = 0; i < 1000; ++i) {
		const auto n = static_cast<int64_t>(rng.getSizeT(0, std::numeric_limits<size_t>::max()));
		EXPECT_EQ(n, convertBackAndForth(n));
	}
}

TEST(Serializer, UInt64Conversion)
{
	// Try individual byte ranges
	EXPECT_EQ(static_cast<uint64_t>(100), convertBackAndForth(static_cast<uint64_t>(100))); // 7
	EXPECT_EQ(static_cast<uint64_t>(12800), convertBackAndForth(static_cast<uint64_t>(12800))); // 14
	EXPECT_EQ(static_cast<uint64_t>(1638400), convertBackAndForth(static_cast<uint64_t>(1638400))); // 21
	EXPECT_EQ(static_cast<uint64_t>(209715200), convertBackAndForth(static_cast<uint64_t>(209715200))); // 28
	EXPECT_EQ(static_cast<uint64_t>(26843545600), convertBackAndForth(static_cast<uint64_t>(26843545600))); // 35
	EXPECT_EQ(static_cast<uint64_t>(3435973836800), convertBackAndForth(static_cast<uint64_t>(3435973836800))); // 42
	EXPECT_EQ(static_cast<uint64_t>(439804651110400), convertBackAndForth(static_cast<uint64_t>(439804651110400))); // 49
	EXPECT_EQ(static_cast<uint64_t>(56294995342131200), convertBackAndForth(static_cast<uint64_t>(56294995342131200))); // 56
	EXPECT_EQ(static_cast<uint64_t>(7205759403792793600), convertBackAndForth(static_cast<uint64_t>(7205759403792793600))); // 64

	testRange(static_cast<uint64_t>(0), static_cast<uint64_t>(1000));
	testRange(std::numeric_limits<uint64_t>::max() - 1000, std::numeric_limits<uint64_t>::max());

	auto& rng = Random::getGlobal();
	for (int i = 0; i < 1000; ++i) {
		const auto n = static_cast<uint64_t>(rng.getSizeT(0, std::numeric_limits<size_t>::max()));
		EXPECT_EQ(n, convertBackAndForth(n));
	}
}
