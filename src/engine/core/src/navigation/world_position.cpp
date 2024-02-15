#include "halley/navigation/world_position.h"

using namespace Halley;

WorldPosition::WorldPosition(const ConfigNode& node, Vector2f defaultPos, int defaultSubWorld)
{
	pos = defaultPos;
	subWorld = defaultSubWorld;

	if (node.getType() == ConfigNodeType::Float2 || node.getType() == ConfigNodeType::Int2) {
		pos = node.asVector2f();
	} else if (node.getType() == ConfigNodeType::Sequence) {
		const auto& v = node.asSequence();
		if (!v.empty()) {
			pos.x = v[0].asFloat(defaultPos.x);
			if (v.size() > 1) {
				pos.y = v[1].asFloat(defaultPos.y);
				if (v.size() > 2) {
					subWorld = v[2].asInt(defaultSubWorld);
				}
			}
		}
	}
}

ConfigNode WorldPosition::toConfigNode() const
{
	ConfigNode::SequenceType seq;
	seq.push_back(ConfigNode(pos.x));
	seq.push_back(ConfigNode(pos.y));
	seq.push_back(ConfigNode(subWorld));
	return seq;
}

String WorldPosition::toString() const
{
	return Halley::toString(pos) + ":" + Halley::toString(subWorld);
}

WorldPosition WorldPosition::operator+(const Vector2f& other) const
{
	return WorldPosition(pos + other, subWorld);
}

WorldPosition WorldPosition::operator-(const Vector2f& other) const
{
	return WorldPosition(pos - other, subWorld);
}

WorldPosition WorldPosition::operator+(const WorldPosition& other) const
{
	return WorldPosition(pos + other.pos, subWorld);
}

WorldPosition WorldPosition::operator-(const WorldPosition& other) const
{
	return WorldPosition(pos - other.pos, subWorld);
}

WorldPosition WorldPosition::operator*(float value) const
{
	return WorldPosition(pos * value, subWorld);
}

WorldPosition WorldPosition::operator/(float value) const
{
	return WorldPosition(pos / value, subWorld);
}

bool WorldPosition::operator==(const WorldPosition& other) const
{
	return pos == other.pos && subWorld == other.subWorld;
}

bool WorldPosition::operator!=(const WorldPosition& other) const
{
	return pos != other.pos || subWorld != other.subWorld;
}

void WorldPosition::serialize(Serializer& s) const
{
	s << pos;
	s << subWorld;
}

void WorldPosition::deserialize(Deserializer& s)
{
	s >> pos;
	s >> subWorld;
}

ConfigNode ConfigNodeSerializer<WorldPosition>::serialize(const WorldPosition& target, const EntitySerializationContext& context)
{
	return target.toConfigNode();
}

WorldPosition ConfigNodeSerializer<WorldPosition>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return WorldPosition(node);
}

