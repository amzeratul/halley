#pragma once

#include "halley/data_structures/config_node.h"
#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"

namespace Halley {
    class RenderGraphDefinition : public Resource {
    public:
        class Node {
        public:
	        String id;
        	String method;
        	ConfigNode methodParameters;

        	Node() = default;
        	Node(const ConfigNode& node);

        	void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
        };

    	class Connection {
    	public:
    		String fromId;
    		String toId;
    		uint8_t fromPin = 0;
    		uint8_t toPin = 0;

    		Connection() = default;
        	Connection(const ConfigNode& node);

    		void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
    	};
    	
    	RenderGraphDefinition() = default;
        explicit RenderGraphDefinition(const ConfigNode& config);
    	
        static std::unique_ptr<RenderGraphDefinition> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::RenderGraphDefinition; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

        gsl::span<const Node> getNodes() const { return nodes; }
    	gsl::span<const Connection> getConnections() const { return connections; }

    private:
    	std::vector<Node> nodes;
    	std::vector<Connection> connections;
    };
}
