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

bool BaseGraph::connectPins(GraphNodeId srcNodeIdx, GraphPinId srcPinN, GraphNodeId dstNodeIdx, GraphPinId dstPinN) {
	auto& srcNode = getNode(srcNodeIdx);
	auto& srcPin = srcNode.getPin(srcPinN);
	auto& dstNode = getNode(dstNodeIdx);
	auto& dstPin = dstNode.getPin(dstPinN);

	for (const auto& conn: srcPin.connections) {
		if (conn.dstNode == dstNodeIdx && conn.dstPin == dstPinN) {
			return false;
		}
	}

	disconnectPinIfSingleConnection(srcNodeIdx, srcPinN);
	disconnectPinIfSingleConnection(dstNodeIdx, dstPinN);
			
	srcPin.connections.emplace_back(BaseGraphNode::PinConnection{ dstNodeIdx, dstPinN });
	dstPin.connections.emplace_back(BaseGraphNode::PinConnection{ srcNodeIdx, srcPinN });

	return true;
}

bool BaseGraph::disconnectPin(GraphNodeId nodeIdx, GraphPinId pinN) {
	auto& node = getNode(nodeIdx);
	auto& pin = node.getPin(pinN);
	if (pin.connections.empty()) {
		return false;
	}

	for (auto& conn: pin.connections) {
		if (conn.dstNode) {
			auto& otherNode = getNode(conn.dstNode.value());
			auto& ocs = otherNode.getPin(conn.dstPin).connections;
			std_ex::erase_if(ocs, [&] (const auto& oc) { return oc.dstNode == nodeIdx && oc.dstPin == pinN; });
		}
	}

	pin.connections.clear();

	return true;
}

bool BaseGraph::disconnectPinIfSingleConnection(GraphNodeId nodeIdx, GraphPinId pinN) {
	auto& node = getNode(nodeIdx);
	if (isMultiConnection(node.getPinType(pinN))) {
		return false;
	}

	return disconnectPin(nodeIdx, pinN);
}

void BaseGraph::validateNodePins(GraphNodeId nodeIdx) {
	auto& node = getNode(nodeIdx);

	const size_t nPinsCur = node.getPins().size();
	const size_t nPinsTarget = node.getPinConfiguration().size();
	if (nPinsCur > nPinsTarget) {
		for (size_t i = nPinsTarget; i < nPinsCur; ++i) {
			disconnectPin(nodeIdx, static_cast<GraphPinId>(i));
		}
		node.getPins().resize(nPinsTarget);
	}
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