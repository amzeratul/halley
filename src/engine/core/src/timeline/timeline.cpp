#include "halley/timeline/timeline.h"

using namespace Halley;

Timeline::Timeline(const ConfigNode& node)
{
	load(node);
}

void Timeline::load(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		sequences = node["sequences"].asVector<TimelineSequence>({});
	}
}

ConfigNode Timeline::toConfigNode() const
{
	ConfigNode::MapType result;

	result["sequences"] = sequences;

	return result;
}

ConfigNode ConfigNodeSerializer<Timeline>::serialize(const Timeline& timeline, const EntitySerializationContext& context)
{
	return timeline.toConfigNode();
}

Timeline ConfigNodeSerializer<Timeline>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return Timeline(node);
}

void ConfigNodeSerializer<Timeline>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, Timeline& target)
{
	target.load(node);
}
