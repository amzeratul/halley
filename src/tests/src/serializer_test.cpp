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
		T result = {};
		ds >> result;
		return result;
	}

	template <typename T>
	static void testSerialization(const T& v)
	{
		EXPECT_EQ(v, convertBackAndForth(v));
	}

	template <typename T>
	static void testRange(T minV, T maxV)
	{
		for (T i = minV; ; ++i) {
			testSerialization(i);
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
			testSerialization(n);
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
	testSerialization(static_cast<uint64_t>(100)); // 7
	testSerialization(static_cast<uint64_t>(12800)); // 14
	testSerialization(static_cast<uint64_t>(1638400)); // 21
	testSerialization(static_cast<uint64_t>(209715200)); // 28
	testSerialization(static_cast<uint64_t>(26843545600)); // 35
	testSerialization(static_cast<uint64_t>(3435973836800)); // 42
	testSerialization(static_cast<uint64_t>(439804651110400)); // 49
	testSerialization(static_cast<uint64_t>(56294995342131200)); // 56
	testSerialization(static_cast<uint64_t>(7205759403792793600)); // 64

	testInt<uint64_t>();
}

TEST(Serializer, Int64Conversion)
{
	testInt<int64_t>();
}

TEST(Serializer, FloatConversion)
{
	testSerialization(0.0f);
	testSerialization(1.0f);
	testSerialization(-1.0f);
	testSerialization(1000000.0f);
	testSerialization(-1000000.0f);
	testSerialization(0.5f);
	testSerialization(-0.5f);
	testSerialization(0.125f);
	testSerialization(-0.125f);
}

TEST(Serializer, DoubleConversion)
{
	testSerialization(0.0);
	testSerialization(1.0);
	testSerialization(-1.0);
	testSerialization(1000000.0);
	testSerialization(-1000000.0);
	testSerialization(0.5);
	testSerialization(-0.5);
	testSerialization(0.125);
	testSerialization(-0.125);
}

TEST(Serializer, BoolConversion)
{
	testSerialization(true);
	testSerialization(false);
}

TEST(Serializer, StringConversion)
{
	testSerialization(String());
	testSerialization(String("Hello world"));
}

TEST(Serializer, HashTest)
{
	auto options = SerializerOptions(SerializerOptions::maxVersion);
	options.toHash = true;

	uint64_t hash;
	{
		auto s = Serializer(options);
		s << String("Hello world");
		s << 1;
		hash = s.getHashDigest();
	}

	{
		auto s = Serializer(options);
		s << String("Hello world");
		s << 1;
		EXPECT_EQ(hash, s.getHashDigest());
	}
}