#include "halley/graphics/render_target/render_graph_definition.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/graph/base_graph_type.h"
#include "halley/resources/resources.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/render_target/render_graph_node_types.h"

using namespace Halley;

RenderGraphNodeDefinition::RenderGraphNodeDefinition(const String& type, const Vector2f& position)
	: BaseGraphNode(type, position)
{
}

RenderGraphNodeDefinition::RenderGraphNodeDefinition(const ConfigNode& node)
	: BaseGraphNode(node)
{
}

std::unique_ptr<BaseGraphNode> RenderGraphNodeDefinition::clone() const
{
	return std::make_unique<RenderGraphNodeDefinition>(*this);
}

void RenderGraphNodeDefinition::assignType(const GraphNodeTypeCollection& nodeTypeCollection) const
{
	nodeType = nodeTypeCollection.tryGetGraphNodeType(type);
}

void RenderGraphNodeDefinition::clearType() const
{
	nodeType = nullptr;
}

const IGraphNodeType& RenderGraphNodeDefinition::getGraphNodeType() const
{
	return *nodeType;
}

void RenderGraphNodeDefinition::loadMaterials(Resources& resources)
{
	dynamic_cast<const RenderGraphNodeType*>(nodeType)->loadMaterials(*this, resources);
}

void RenderGraphNodeDefinition::setMaterial(std::shared_ptr<const MaterialDefinition> material)
{
	this->material = std::move(material);
}

const std::shared_ptr<const MaterialDefinition>& RenderGraphNodeDefinition::getMaterial() const
{
	return material;
}

uint8_t RenderGraphNodeDefinition::getPinIndex(GraphPinId pinId, GraphNodePinDirection direction) const
{
	uint8_t result = 0;
	const auto& pins = getPinConfiguration();
	for (size_t i = 0; i < pinId; ++i) {
		if (pins[i].direction == direction) {
			++result;
		}
	}
	return result;
}


RenderGraphDefinition::RenderGraphDefinition(const ConfigNode& node)
{
	load(node);
}

RenderGraphDefinition::RenderGraphDefinition(const ConfigNode& node, Resources& resources)
{
	RenderGraphDefinition::load(node, resources);
}

void RenderGraphDefinition::load(const ConfigNode& node)
{
	nodes = node["nodes"].asVector<RenderGraphNodeDefinition>({});
}

void RenderGraphDefinition::load(const ConfigNode& node, Resources& resources)
{
	load(node);
	loadMaterials(resources);
}

GraphNodeId RenderGraphDefinition::addNode(const String& type, Vector2f pos, ConfigNode settings)
{
	const auto id = static_cast<GraphNodeId>(nodes.size());
	nodes.push_back(RenderGraphNodeDefinition(type, pos));
	nodes.back().getSettings() = settings;
	finishGraph();
	return id;
}

ConfigNode RenderGraphDefinition::toConfigNode() const
{
	ConfigNode::MapType result;
	result["nodes"] = nodes;
	return result;
}

std::shared_ptr<RenderGraphDefinition> RenderGraphDefinition::loadResource(ResourceLoader& loader)
{
	auto graph = std::make_shared<RenderGraphDefinition>();
	Deserializer::fromBytes(*graph, loader.getStatic()->getSpan(), SerializerOptions(SerializerOptions::maxVersion));
	graph->loadMaterials(loader.getResources());
	return graph;
}

void RenderGraphDefinition::reload(Resource&& resource)
{
	*this = dynamic_cast<RenderGraphDefinition&&>(resource);
}

void RenderGraphDefinition::serialize(Serializer& s) const
{
	s << nodes;
}

void RenderGraphDefinition::deserialize(Deserializer& s)
{
	s >> nodes;
	finishGraph();
}

void RenderGraphDefinition::loadMaterials(Resources& resources)
{
	assignTypes(*RenderGraphNodeTypes::makeRenderGraphTypes());
	for (auto& node: nodes) {
		node.loadMaterials(resources);
	}
}

bool RenderGraphDefinition::isMultiConnection(GraphNodePinType pinType) const
{
	return (pinType.type == static_cast<int>(RenderGraphElementType::ColourBuffer) && pinType.direction == GraphNodePinDirection::Output)
		|| (pinType.type == static_cast<int>(RenderGraphElementType::DepthStencilBuffer) && pinType.direction == GraphNodePinDirection::Output)
		|| (pinType.type == static_cast<int>(RenderGraphElementType::Dependency))
		|| (pinType.type == static_cast<int>(RenderGraphElementType::Texture) && pinType.direction == GraphNodePinDirection::Output);
}

