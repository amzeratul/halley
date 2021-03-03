#pragma once

#include "halley/data_structures/config_node.h"
#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"

namespace Halley {
	class MaterialDefinition;

	enum class RenderGraphMethod {
    	None,
    	Output,
	    Paint,
    	Screen
    };

	template <>
	struct EnumNames<RenderGraphMethod> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"none",
				"output",
				"paint",
				"screen"
			}};
		}
	};
	
    class RenderGraphDefinition : public Resource {
    public:
        class Node {
        public:        	
	        String id;
        	RenderGraphMethod method = RenderGraphMethod::None;
        	ConfigNode methodParameters;
        	
        	std::shared_ptr<const MaterialDefinition> material;

        	Node() = default;
        	Node(const ConfigNode& node);

        	void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

        	void loadMaterials(Resources& resources);
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

    	void loadMaterials(Resources& resources);
    };
}
