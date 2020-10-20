#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

static int convertBackAndForth(int v)
{
	static gsl::byte bytes[128];
	Serializer s(bytes, SerializerOptions(SerializerOptions::maxVersion));
	s << v;
	
	Deserializer ds(gsl::span<gsl::byte>(bytes, s.getPosition()), SerializerOptions(SerializerOptions::maxVersion));
	int result = 0;
	ds >> result;
	return result;
}

TEST(Serializer, IntConversion)
{
	//EXPECT_EQ(63, convertBackAndForth(63));
	//EXPECT_EQ(65, convertBackAndForth(65));
	//EXPECT_EQ(64, convertBackAndForth(64));
	
	for (int j = 0; j < 2; ++j) {
		for (int i = 0; i < 10000; ++i) {
			int value = (j == 0 ? 1 : -1) * i;
			EXPECT_EQ(value, convertBackAndForth(value));
		}
	}
}