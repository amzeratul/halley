#include "halley/timeline/timeline.h"

using namespace Halley;

Timeline::Timeline(const ConfigNode& node)
{
	load(node);
}

void Timeline::load(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		entities = node["entities"].asVector<TimelineSequenceEntity>({});
	}
}

ConfigNode Timeline::toConfigNode() const
{
	ConfigNode::MapType result;

	result["entities"] = entities;

	return result;
}

bool Timeline::hasEntity(std::string_view entityId) const
{
	return std::any_of(entities.begin(), entities.end(), [&](const auto& e) { return e.entityId == entityId; });
}

Vector<TimelineSequence>& Timeline::getSequences(std::string_view entityId)
{
	for (auto& s: entities) {
		if (s.entityId == entityId) {
			return s.sequences;
		}
	}
	entities.emplace_back(entityId);
	return entities.back().sequences;
}

const Vector<TimelineSequence>& Timeline::getSequences(std::string_view entityId) const
{
	for (auto& s: entities) {
		if (s.entityId == entityId) {
			return s.sequences;
		}
	}
	throw Exception("TimelineSequence not found: " + String(entityId), HalleyExceptions::Tools);
}

TimelineSequence& Timeline::getSequence(std::string_view entityId, const TimelineSequence::Key& key)
{
	auto& sequences = getSequences(entityId);
	for (auto& s: sequences) {
		if (s.getKey() == key) {
			return s;
		}
	}
	sequences.emplace_back(key);
	return sequences.back();
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
