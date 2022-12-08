#include "halley/entity/world_position.h"

#include "components/transform_2d_component.h"

using namespace Halley;

WorldPosition::WorldPosition(const ConfigNode& node, Vector2f defaultPos, int defaultSubWorld)
{
	pos = defaultPos;
	subWorld = defaultSubWorld;

	if (node.getType() == ConfigNodeType::Float2 || node.getType() == ConfigNodeType::Int2) {
		pos = node.asVector2f();
	} else if (node.getType() == ConfigNodeType::Sequence) {
		const auto& v = node.asSequence();
		if (v.size() > 0) {
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

WorldPosition::WorldPosition(Transform2DComponent& transform2D)
{
	pos = transform2D.getGlobalPosition();
	subWorld = transform2D.getSubWorld();
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
