#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

template <typename T>
static uint64_t hashNonConstexpr(const T& v)
{
	return Hash::Detail::hashNBytes(reinterpret_cast<const void*>(&v), sizeof(v));
}

template <typename T>
consteval static uint64_t hashConsteval(const T& v)
{
	return Hash::hash(v);
}

TEST(HashTest, HashConflict)
{
	Vector<uint64_t> results;

	constexpr int n = 10000;
	results.reserve(n);
	for (int i = 0; i < n; ++i) {
		results += hashNonConstexpr(i);
	}

	std::sort(results.begin(), results.end());
	for (int i = 1; i < n; ++i) {
		EXPECT_NE(results[i - 1], results[i]);
	}
}

TEST(HashTest, HashSizeDifference)
{
	const auto a = hashNonConstexpr(uint8_t(1));
	const auto b = hashNonConstexpr(uint16_t(1));
	const auto c = hashNonConstexpr(uint32_t(1));
	const auto d = hashNonConstexpr(uint64_t(1));
	EXPECT_NE(a, b);
	EXPECT_NE(a, c);
	EXPECT_NE(a, d);
	EXPECT_NE(b, c);
	EXPECT_NE(b, d);
	EXPECT_NE(c, d);
}

TEST(HashTest, HashString)
{
	EXPECT_NE(Hash::hash(std::string_view("Hello")), Hash::hash(std::string_view("world")));
	EXPECT_EQ(Hash::Detail::hashNBytes("Hello", 5), Hash::hash(std::string_view("Hello")));
	EXPECT_EQ(Halley::hash<String>()(String("Hello")), Hash::hash(std::string_view("Hello")));
	EXPECT_EQ(Halley::hash<std::string>()(std::string("Hello")), Hash::hash(std::string_view("Hello")));
}

TEST(HashTest, Consteval)
{
	EXPECT_EQ(hashConsteval(uint8_t(1)), hashNonConstexpr(uint8_t(1)));
	EXPECT_EQ(hashConsteval(uint16_t(1)), hashNonConstexpr(uint16_t(1)));
	EXPECT_EQ(hashConsteval(uint32_t(1)), hashNonConstexpr(uint32_t(1)));
	EXPECT_EQ(hashConsteval(uint64_t(1)), hashNonConstexpr(uint64_t(1)));
	EXPECT_EQ(hashConsteval(int8_t(1)), hashNonConstexpr(int8_t(1)));
	EXPECT_EQ(hashConsteval(int16_t(1)), hashNonConstexpr(int16_t(1)));
	EXPECT_EQ(hashConsteval(int32_t(1)), hashNonConstexpr(int32_t(1)));
	EXPECT_EQ(hashConsteval(int64_t(1)), hashNonConstexpr(int64_t(1)));
	EXPECT_EQ(hashConsteval(std::string_view("Hello")), Hash::hash(std::string_view("Hello")));
}
