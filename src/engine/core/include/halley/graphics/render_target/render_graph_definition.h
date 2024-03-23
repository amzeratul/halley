#pragma once

#include "halley/data_structures/config_node.h"
#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"
#include "render_graph_pin_type.h"
#include "halley/graph/base_graph.h"

namespace Halley {
	class MaterialDefinition;

	enum class RenderGraphMethod {
    	None,
    	Output,
		ImageOutput,
	    Paint,
    	Overlay,
		RenderToTexture
    };

	template <>
	struct EnumNames<RenderGraphMethod> {
		constexpr std::array<const char*, 6> operator()() const {
			return{{
				"none",
				"output",
				"imageOutput",
				"paint",
				"overlay",
				"renderToTexture"
			}};
		}
	};
	
    class RenderGraphDefinition : public Resource {
    public:
        class Node {
        public:        	
	        String id;
			int priority;
        	RenderGraphMethod method = RenderGraphMethod::None;
        	ConfigNode methodParameters;
        	Vector2f position;
        	
        	std::shared_ptr<const MaterialDefinition> material;

        	Node() = default;
        	Node(const ConfigNode& node);

        	void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

        	void loadMaterials(Resources& resources);

        	gsl::span<const RenderGraphPinType> getInputPins() const { return inputPins; }
        	gsl::span<const RenderGraphPinType> getOutputPins() const { return outputPins; }

        private:
            void generatePins();
        	
        	Vector<RenderGraphPinType> inputPins;
        	Vector<RenderGraphPinType> outputPins;
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

        gsl::span<const Node> getNodes() const { return nodes; }
    	gsl::span<const Connection> getConnections() const { return connections; }
    	
        static std::unique_ptr<RenderGraphDefinition> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::RenderGraphDefinition; }
    	void reload(Resource&& resource) override;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

    private:
    	Vector<Node> nodes;
    	Vector<Connection> connections;

    	void loadMaterials(Resources& resources);
    };

    class RenderGraphNode2 : public BaseGraphNode {
    public:
	    GraphNodePinType getPinType(GraphPinId idx) const override;
	    gsl::span<const GraphNodePinType> getPinConfiguration() const override;
	    std::unique_ptr<BaseGraphNode> clone() const override;
    };

    class RenderGraphDefinition2 : public BaseGraphImpl<RenderGraphNode2> {
    public:
	    GraphNodeId addNode(const String& type, Vector2f pos, ConfigNode settings) override;
	    void load(const ConfigNode& node) override;
	    ConfigNode toConfigNode() const override;

        static std::shared_ptr<RenderGraphDefinition2> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::RenderGraphDefinition; }
    	void reload(Resource&& resource) override;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
    };
}
