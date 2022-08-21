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
		virtual gsl::span<const GraphNodePinType> getPinConfiguration() const = 0;

		virtual void feedToHash(Hash::Hasher& hasher);

	protected:
		ConfigNode settings;
		Vector<Pin> pins;
		String type;
		Vector2f position;
		GraphNodeId id = 0;
	};


	class BaseGraph {
	public:
		virtual ~BaseGraph() = default;

		virtual GraphNodeId addNode(const String& type, Vector2f pos, ConfigNode settings) = 0;

		bool connectPins(GraphNodeId srcNodeIdx, GraphPinId srcPinN, GraphNodeId dstNodeIdx, GraphPinId dstPinN);
		bool disconnectPin(GraphNodeId nodeIdx, GraphPinId pinN);
		bool disconnectPinIfSingleConnection(GraphNodeId nodeIdx, GraphPinId pinN);
		void validateNodePins(GraphNodeId nodeIdx);

		virtual BaseGraphNode& getNode(size_t i) = 0;
		virtual const BaseGraphNode& getNode(size_t i) const = 0;

	protected:
		virtual bool isMultiConnection(GraphNodePinType pinType) const
		{
			return false;
		}
	};


	template <typename NodeType>
	class BaseGraphImpl : public BaseGraph {
	public:
		static_assert(std::is_base_of_v<BaseGraphNode, NodeType>);
		
		const Vector<NodeType>& getNodes() const
		{
			return nodes;
		}

		Vector<NodeType>& getNodes()
		{
			return nodes;
		}

		BaseGraphNode& getNode(size_t i) final override
		{
			return nodes.at(i);
		}

		const BaseGraphNode& getNode(size_t i) const final override
		{
			return nodes.at(i);
		}

	protected:
		Vector<NodeType> nodes;
	};
}
