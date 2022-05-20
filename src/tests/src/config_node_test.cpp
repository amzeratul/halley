#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

TEST(HalleyConfigNode, Int)
{
	ConfigNode node;
	node = 42;
	
	EXPECT_EQ(node.asInt(), 42);
}

TEST(HalleyConfigNode, Map)
{
	ConfigNode node;
	node = ConfigNode::MapType();
	node["hello"] = "world";

	EXPECT_TRUE(node.hasKey("hello"));
	EXPECT_FALSE(node.hasKey("world"));
	EXPECT_EQ(node["hello"].asString(), "world");
}

TEST(HalleyConfigNode, Sequence)
{
	auto seq = ConfigNode::SequenceType();
	seq.push_back(ConfigNode(Vector2i(3, 4)));
	ConfigNode node = std::move(seq);

	EXPECT_TRUE(node.getType() == ConfigNodeType::Sequence);
	EXPECT_EQ(node.asSequence().size(), 1);
}
