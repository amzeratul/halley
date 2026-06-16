#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

namespace {
	template <typename T>
	static T convertBackAndForth(T v)
	{
		static std::byte bytes[128];
		Serializer s(bytes, SerializerOptions(SerializerOptions::maxVersion));
		s << v;
		
		Deserializer ds(gsl::span<std::byte>(bytes, s.getPosition()), SerializerOptions(SerializerOptions::maxVersion));
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

	template <typename T>
	static void testInt()
	{
		static_assert(sizeof(T) >= 2);

		auto rng = Random(Random::getSharedGlobal().getRawInt64());

		if constexpr (std::is_signed_v<T>) {
			testRange<T>(static_cast<T>(-1000), static_cast<T>(1000));
		} else {
			testRange<T>(static_cast<T>(0), static_cast<T>(2000));
		}

		testRange<T>(std::numeric_limits<T>::min(), std::numeric_limits<T>::min() + static_cast<T>(1000));
		testRange<T>(std::numeric_limits<T>::max() - static_cast<T>(1000), std::numeric_limits<T>::max());

		for (int i = 0; i < 1000; ++i) {
			const auto n = static_cast<T>(rng.getInt(std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
			EXPECT_EQ(n, convertBackAndForth<T>(n));
		}
	}
}

TEST(Serializer, UInt8Conversion)
{
	testRange(static_cast<uint8_t>(0), static_cast<uint8_t>(255));
}

TEST(Serializer, Int8Conversion)
{
	testRange(static_cast<int8_t>(-128), static_cast<int8_t>(127));
}

TEST(Serializer, UInt16Conversion)
{
	testInt<uint16_t>();
}

TEST(Serializer, Int16Conversion)
{
	testInt<int16_t>();
}

TEST(Serializer, UInt32Conversion)
{
	testInt<uint32_t>();
}

TEST(Serializer, Int32Conversion)
{
	testInt<int32_t>();
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

	testInt<uint64_t>();
}

TEST(Serializer, Int64Conversion)
{
	testInt<int64_t>();
}
