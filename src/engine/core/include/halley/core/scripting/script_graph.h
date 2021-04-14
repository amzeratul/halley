#pragma once
#include "halley/bytes/config_node_serializer.h"

namespace Halley {
	class ScriptGraphNode {
	public:
		ScriptGraphNode();
		ScriptGraphNode(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		Vector2f getPosition() const { return position; }
		void setPosition(Vector2f p) { position = p; }

	private:
		Vector2f position;
	};
	
	class ScriptGraph {
	public:
		ScriptGraph();
		ScriptGraph(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		const std::vector<ScriptGraphNode>& getNodes() const { return nodes; }
		std::vector<ScriptGraphNode>& getNodes() { return nodes; }

	private:
		std::vector<ScriptGraphNode> nodes;
    };

	template<>
	class ConfigNodeSerializer<ScriptGraph> {
	public:
		ConfigNode serialize(const ScriptGraph& graph, const ConfigNodeSerializationContext& context);
		ScriptGraph deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node);
	};
}
