#include "halley/core/graph/base_graph.h"
#include "halley/utils/hash.h"
#include "halley/bytes/byte_serializer.h"

using namespace Halley;


BaseGraphNode::BaseGraphNode()
{}

BaseGraphNode::BaseGraphNode(String type, Vector2f position)
	: settings(ConfigNode::MapType())
	, type(std::move(type))
	, position(position)
{
}

BaseGraphNode::BaseGraphNode(const ConfigNode& node)
{
	position = node["position"].asVector2f();
	type = node["type"].asString();
	if (node.hasKey("settings")) {
		settings = ConfigNode(node["settings"]);
	} else {
		settings = ConfigNode::MapType();
	}
	pins = node["pins"].asVector<Pin>();
}

ConfigNode BaseGraphNode::toConfigNode() const
{
	ConfigNode::MapType result;
	result["position"] = position;
	result["type"] = type;
	if (settings.getType() == ConfigNodeType::Map && !settings.asMap().empty()) {
		result["settings"] = ConfigNode(settings);
	}
	result["pins"] = pins;
	return result;
}

void BaseGraphNode::serialize(Serializer& s) const
{
	s << position;
	s << type;
	s << settings;
	s << pins;
}

void BaseGraphNode::deserialize(Deserializer& s)
{
	s >> position;
	s >> type;
	s >> settings;
	s >> pins;
}

void BaseGraphNode::feedToHash(Hash::Hasher& hasher)
{
	hasher.feed(type);
	// TODO: settings, pins
}

void BaseGraphNode::onNodeRemoved(GraphNodeId nodeId)
{
	for (auto& pin: pins) {
		for (auto& o: pin.connections) {
			if (o.dstNode) {
				if (o.dstNode.value() == nodeId) {
					o.dstNode = OptionalLite<GraphNodeId>();
					o.dstPin = 0;
				} else if (o.dstNode.value() >= nodeId) {
					--o.dstNode.value();
				}
			}
		}
		std_ex::erase_if(pin.connections, [] (const PinConnection& c) { return !c.hasConnection(); });
	}
}

void BaseGraphNode::remapNodes(const HashMap<GraphNodeId, GraphNodeId>& remap)
{
	for (auto& pin: pins) {
		for (auto& o: pin.connections) {
			if (o.dstNode) {
				const auto iter = remap.find(o.dstNode.value());
				if (iter != remap.end()) {
					o.dstNode = iter->second;
				} else {
					o.dstNode.reset();
					o.dstPin = 0;
				}
			} else if (o.entityIdx) {
				o.entityIdx.reset();
			}
		}
		std_ex::erase_if(pin.connections, [] (const PinConnection& c) { return !c.hasConnection(); });
	}
}

void BaseGraphNode::offsetNodes(GraphNodeId offset)
{
	for (auto& pin: pins) {
		for (auto& o: pin.connections) {
			if (o.dstNode) {
				*o.dstNode += offset;
			}
		}
	}
	id += offset;
}
