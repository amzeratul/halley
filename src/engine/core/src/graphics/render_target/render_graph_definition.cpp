#include "halley/graphics/render_target/render_graph_definition.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/resources.h"
#include "halley/graphics/material/material_definition.h"

using namespace Halley;

RenderGraphDefinition::RenderGraphDefinition(const ConfigNode& config)
{
	for (const auto& node: config["nodes"].asSequence()) {
		nodes.emplace_back(node);
	}
	for (const auto& connection: config["connections"].asSequence()) {
		connections.emplace_back(connection);
	}
}

RenderGraphDefinition::Node::Node(const ConfigNode& node)
{
	id = node["id"].asString();
	priority = node["priority"].asInt(0);
	method = fromString<RenderGraphMethod>(node["method"].asString());
	methodParameters = ConfigNode(node["methodParameters"]);
	position = node["position"].asVector2f(Vector2f(100, 100));
}

RenderGraphDefinition::Connection::Connection(const ConfigNode& node)
{
	const auto& fromNode = node["from"];
	const auto& toNode = node["to"];
	fromId = fromNode["node"].asString();
	fromPin = gsl::narrow_cast<uint8_t>(fromNode["pin"].asInt());
	toId = toNode["node"].asString();
	toPin = gsl::narrow_cast<uint8_t>(toNode["pin"].asInt());
}

void RenderGraphDefinition::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<RenderGraphDefinition&>(resource));
}

void RenderGraphDefinition::serialize(Serializer& s) const
{
	s << nodes;
	s << connections;
}

void RenderGraphDefinition::deserialize(Deserializer& s)
{
	s >> nodes;
	s >> connections;
}

void RenderGraphDefinition::Node::serialize(Serializer& s) const
{
	s << id;
	s << priority;
	s << method;
	s << methodParameters;
	s << position;
}

void RenderGraphDefinition::Node::deserialize(Deserializer& s)
{
	s >> id;
	s >> priority;
	s >> method;
	s >> methodParameters;
	s >> position;
}

void RenderGraphDefinition::Connection::serialize(Serializer& s) const
{
	s << fromId;
	s << toId;
	s << fromPin;
	s << toPin;
}

void RenderGraphDefinition::Connection::deserialize(Deserializer& s)
{
	s >> fromId;
	s >> toId;
	s >> fromPin;
	s >> toPin;
}

std::unique_ptr<RenderGraphDefinition> RenderGraphDefinition::loadResource(ResourceLoader& loader)
{
	auto result = std::make_unique<RenderGraphDefinition>();
	Deserializer::fromBytes(*result, loader.getStatic()->getSpan());
	result->loadMaterials(loader.getResources());
	return result;
}

void RenderGraphDefinition::loadMaterials(Resources& resources)
{
	for (auto& node: nodes) {
		node.loadMaterials(resources);
	}
}

void RenderGraphDefinition::Node::loadMaterials(Resources& resources)
{
	if (method == RenderGraphMethod::Overlay) {
		material = resources.get<MaterialDefinition>(methodParameters["material"].asString());
	}
	generatePins();
}

void RenderGraphDefinition::Node::generatePins()
{
	inputPins = {{ RenderGraphPinType::ColourBuffer, RenderGraphPinType::DepthStencilBuffer }};
	if (method == RenderGraphMethod::Output) {
		outputPins.clear();
	} else if (method == RenderGraphMethod::ImageOutput) {
		inputPins = {{ RenderGraphPinType::Texture }};
	} else if (method == RenderGraphMethod::RenderToTexture) {
		inputPins = {{ RenderGraphPinType::ColourBuffer }};
		outputPins = {{ RenderGraphPinType::Dependency }};
	} else if (method == RenderGraphMethod::Paint) {
		inputPins.push_back(RenderGraphPinType::Dependency);
		outputPins = {{ RenderGraphPinType::ColourBuffer, RenderGraphPinType::DepthStencilBuffer }};
	} else {
		outputPins = {{ RenderGraphPinType::ColourBuffer, RenderGraphPinType::DepthStencilBuffer }};
	}

	if (material) {
		for (const auto& t: material->getTextures()) {
			inputPins.push_back(RenderGraphPinType::Texture);
		}
	}
}








GraphNodePinType RenderGraphNode2::getPinType(GraphPinId idx) const
{
	// TODO
	return {};
}

gsl::span<const GraphNodePinType> RenderGraphNode2::getPinConfiguration() const
{
	// TODO
	return {};
}

std::unique_ptr<BaseGraphNode> RenderGraphNode2::clone() const
{
	return std::make_unique<RenderGraphNode2>(*this);
}


GraphNodeId RenderGraphDefinition2::addNode(const String& type, Vector2f pos, ConfigNode settings)
{
	// TODO
	const auto id = static_cast<GraphNodeId>(nodes.size());
	nodes.push_back(RenderGraphNode2());
	return id;
}

void RenderGraphDefinition2::load(const ConfigNode& node)
{
	// TODO
}

ConfigNode RenderGraphDefinition2::toConfigNode() const
{
	ConfigNode::MapType result;
	// TODO
	return result;
}

std::shared_ptr<RenderGraphDefinition2> RenderGraphDefinition2::loadResource(ResourceLoader& loader)
{
	auto graph = std::make_shared<RenderGraphDefinition2>();
	Deserializer::fromBytes(*graph, loader.getStatic()->getSpan(), SerializerOptions(SerializerOptions::maxVersion));
	return graph;
}

void RenderGraphDefinition2::reload(Resource&& resource)
{
	*this = dynamic_cast<RenderGraphDefinition2&&>(resource);
}

void RenderGraphDefinition2::serialize(Serializer& s) const
{
	s << nodes;
}

void RenderGraphDefinition2::deserialize(Deserializer& s)
{
	s >> nodes;
}

