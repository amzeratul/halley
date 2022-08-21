#pragma once

#include "base_graph_enums.h"
#include "halley/data_structures/config_node.h"
#include "halley/data_structures/maybe.h"
#include "halley/utils/algorithm.h"
#include "halley/utils/hash.h"

namespace Halley {
	class BaseGraphNode {
	public:
		struct PinConnection {
			OptionalLite<GraphNodeId> dstNode = {};
			GraphPinId dstPin = 0;
			OptionalLite<uint8_t> entityIdx;

			PinConnection() = default;
			PinConnection(const ConfigNode& node);
			PinConnection(GraphNodeId dstNode, GraphPinId dstPin);
			explicit PinConnection(OptionalLite<uint8_t> entityIdx);

			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

			bool hasConnection() const;
		};
		
		struct Pin {
			Vector<PinConnection> connections;

			Pin() = default;
			Pin(const ConfigNode& node);
			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

			bool hasConnection() const;
		};

		BaseGraphNode();
		BaseGraphNode(String type, Vector2f position);
		BaseGraphNode(const ConfigNode& node);

		virtual ConfigNode toConfigNode() const;

		virtual void serialize(Serializer& s) const;
		virtual void deserialize(Deserializer& s);

		Vector2f getPosition() const { return position; }
		void setPosition(Vector2f p) { position = p; }

		const String& getType() const { return type; }

		Vector<Pin>& getPins() { return pins; }
		const Vector<Pin>& getPins() const { return pins; }
		Pin& getPin(size_t idx)
		{
			if (idx >= pins.size()) {
				pins.resize(idx + 1);
			}
			return pins[idx];
		}
		const Pin& getPin(size_t idx) const
		{
			if (idx >= pins.size()) {
				static Pin dummy;
				return dummy;
			}
			return pins[idx];
		}

		const ConfigNode& getSettings() const { return settings; }
		ConfigNode& getSettings() { return settings; }

		GraphNodeId getId() const { return id; }
		void setId(GraphNodeId i) { id = i; }

		void onNodeRemoved(GraphNodeId nodeId);
		void remapNodes(const HashMap<GraphNodeId, GraphNodeId>& remap);
		virtual void offsetNodes(GraphNodeId offset);

		virtual GraphNodePinType getPinType(GraphPinId idx) const = 0;

		virtual void feedToHash(Hash::Hasher& hasher);

	protected:
		ConfigNode settings;
		Vector<Pin> pins;
		String type;
		Vector2f position;
		GraphNodeId id = 0;
	};

	template <typename NodeType>
	class BaseGraph {
	public:
		const Vector<NodeType>& getNodes() const
		{
			return nodes;
		}

		Vector<NodeType>& getNodes()
		{
			return nodes;
		}

		bool connectPins(GraphNodeId srcNodeIdx, GraphPinId srcPinN, GraphNodeId dstNodeIdx, GraphPinId dstPinN)
		{
			auto& srcNode = nodes.at(srcNodeIdx);
			auto& srcPin = srcNode.getPin(srcPinN);
			auto& dstNode = nodes.at(dstNodeIdx);
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

		bool disconnectPin(GraphNodeId nodeIdx, GraphPinId pinN)
		{
			auto& node = nodes.at(nodeIdx);
			auto& pin = node.getPin(pinN);
			if (pin.connections.empty()) {
				return false;
			}

			for (auto& conn: pin.connections) {
				if (conn.dstNode) {
					auto& otherNode = nodes.at(conn.dstNode.value());
					auto& ocs = otherNode.getPin(conn.dstPin).connections;
					std_ex::erase_if(ocs, [&] (const auto& oc) { return oc.dstNode == nodeIdx && oc.dstPin == pinN; });
				}
			}

			pin.connections.clear();

			return true;
		}

		bool disconnectPinIfSingleConnection(GraphNodeId nodeIdx, GraphPinId pinN)
		{
			auto& node = nodes.at(nodeIdx);
			if (isMultiConnection(node.getPinType(pinN))) {
				return false;
			}

			return disconnectPin(nodeIdx, pinN);
		}

		void validateNodePins(GraphNodeId nodeIdx)
		{
			auto& node = nodes.at(nodeIdx);

			const size_t nPinsCur = node.getPins().size();
			const size_t nPinsTarget = node.getNodeType().getPinConfiguration(node).size();
			if (nPinsCur > nPinsTarget) {
				for (size_t i = nPinsTarget; i < nPinsCur; ++i) {
					disconnectPin(nodeIdx, static_cast<GraphPinId>(i));
				}
				node.getPins().resize(nPinsTarget);
			}
		}

	protected:
		Vector<NodeType> nodes;

		virtual bool isMultiConnection(GraphNodePinType pinType) const
		{
			return false;
		}
	};
}
