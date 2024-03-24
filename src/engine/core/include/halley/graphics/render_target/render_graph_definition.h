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
	
    class RenderGraphNodeDefinition : public BaseGraphNode {
    public:
        RenderGraphNodeDefinition() = default;
        RenderGraphNodeDefinition(const String& type, const Vector2f& position);
        RenderGraphNodeDefinition(const ConfigNode& node);

        ConfigNode toConfigNode() const override;

	    std::unique_ptr<BaseGraphNode> clone() const override;

        void assignType(const GraphNodeTypeCollection& nodeTypeCollection) const override;
        void clearType() const override;
        const IGraphNodeType& getGraphNodeType() const override;

        void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
        
    	void loadMaterials(Resources& resources);

        const String& getName() const;
        void setName(String name);

        void setMaterial(std::shared_ptr<const MaterialDefinition> material);
        const std::shared_ptr<const MaterialDefinition>& getMaterial() const;

    	uint8_t getPinIndex(GraphPinId pinId, GraphNodePinDirection direction) const;

    private:
        String name;
        mutable const IGraphNodeType* nodeType = nullptr;
        std::shared_ptr<const MaterialDefinition> material;
    };

    class RenderGraphDefinition : public BaseGraphImpl<RenderGraphNodeDefinition> {
    public:
        RenderGraphDefinition() = default;
        RenderGraphDefinition(const ConfigNode& node);
        RenderGraphDefinition(const ConfigNode& node, Resources& resources);

	    void load(const ConfigNode& node);
	    void load(const ConfigNode& node, Resources& resources) override;
	    ConfigNode toConfigNode() const override;

    	GraphNodeId addNode(const String& type, Vector2f pos, ConfigNode settings) override;

        static std::shared_ptr<RenderGraphDefinition> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::RenderGraphDefinition; }
    	void reload(Resource&& resource) override;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

    	void loadMaterials(Resources& resources);

    protected:
        bool isMultiConnection(GraphNodePinType pinType) const override;
    };
}
