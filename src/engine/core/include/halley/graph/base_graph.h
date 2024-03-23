#pragma once

#include "base_graph_enums.h"
#include "halley/data_structures/config_node.h"
#include "halley/data_structures/maybe.h"
#include "halley/resources/resource.h"
#include "halley/utils/hash.h"

namespace Halley {
	class BaseGraphNode {
	public:
		struct PinConnection {
			OptionalLite<GraphNodeId> dstNode = {};
			GraphPinId dstPin = 0;

			PinConnection() = default;
			PinConnection(const ConfigNode& node);
			PinConnection(GraphNodeId dstNode, GraphPinId dstPin);

			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

			bool hasConnection() const;

			void feedToHash(Hash::Hasher& hasher) const;
		};
		
		struct Pin {
			Vector<PinConnection> connections;

			Pin() = default;
			Pin(const ConfigNode& node);
			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

			bool hasConnection() const;

			void feedToHash(Hash::Hasher& hasher) const;
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
		virtual std::unique_ptr<BaseGraphNode> clone() const = 0;

		virtual void feedToHash(Hash::Hasher& hasher) const;
		virtual bool canDraw() const { return true; }

	protected:
		ConfigNode settings;
		Vector<Pin> pins;
		String type;
		Vector2f position;
		GraphNodeId id = 0;
	};


	class BaseGraph: public Resource {
	public:
		virtual ~BaseGraph() = default;

		virtual BaseGraphNode& addNode(const BaseGraphNode& node) = 0;
		virtual GraphNodeId addNode(const String& type, Vector2f pos, ConfigNode settings) = 0;

		bool connectPins(GraphNodeId srcNodeIdx, GraphPinId srcPinN, GraphNodeId dstNodeIdx, GraphPinId dstPinN);
		bool disconnectPin(GraphNodeId nodeIdx, GraphPinId pinN);
		bool disconnectPinIfSingleConnection(GraphNodeId nodeIdx, GraphPinId pinN);
		Vector<std::pair<GraphNodeId, GraphPinId>> getPinConnections(GraphNodeId nodeIdx, GraphPinId pinN) const;
		void validateNodePins(GraphNodeId nodeIdx);

		virtual BaseGraphNode& getNode(size_t i) = 0;
		virtual const BaseGraphNode& getNode(size_t i) const = 0;
		virtual size_t getNumNodes() const = 0;
		virtual void eraseNode(size_t i) = 0;

		virtual void finishGraph() {}
		
		virtual void load(const ConfigNode& node) = 0;
		virtual ConfigNode toConfigNode() const = 0;
		virtual String toYAML() const;

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

		BaseGraphNode& addNode(const BaseGraphNode& node) override
		{
			return nodes.emplace_back(dynamic_cast<const NodeType&>(node));
		}

		void eraseNode(size_t i) final override
		{
			nodes.erase(nodes.begin() + i);
		}

		size_t getNumNodes() const override
		{
			return nodes.size();
		}

	protected:
		Vector<NodeType> nodes;
	};
}
