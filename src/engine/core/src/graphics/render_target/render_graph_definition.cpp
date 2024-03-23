#include "halley/graphics/render_target/render_graph_definition.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/graph/base_graph_type.h"
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
	id = node["name"].asString();
	priority = node["priority"].asInt(0);
	method = fromString<RenderGraphMethod>(node["type"].asString());
	methodParameters = ConfigNode(node["settings"]);
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


RenderGraphNode2::RenderGraphNode2(const String& type, const Vector2f& position)
	: BaseGraphNode(type, position)
{
}

RenderGraphNode2::RenderGraphNode2(const ConfigNode& node)
	: BaseGraphNode(node)
{
	name = node["name"].asString("");
}

ConfigNode RenderGraphNode2::toConfigNode() const
{
	auto result = BaseGraphNode::toConfigNode();
	result["name"] = name;
	return result;
}

std::unique_ptr<BaseGraphNode> RenderGraphNode2::clone() const
{
	return std::make_unique<RenderGraphNode2>(*this);
}

void RenderGraphNode2::assignType(const GraphNodeTypeCollection& nodeTypeCollection) const
{
	nodeType = nodeTypeCollection.tryGetGraphNodeType(type);
}

void RenderGraphNode2::clearType() const
{
	nodeType = nullptr;
}

const IGraphNodeType& RenderGraphNode2::getGraphNodeType() const
{
	return *nodeType;
}

void RenderGraphNode2::serialize(Serializer& s) const
{
	BaseGraphNode::serialize(s);
	s << name;
}

void RenderGraphNode2::deserialize(Deserializer& s)
{
	BaseGraphNode::deserialize(s);
	s >> name;
}

const String& RenderGraphNode2::getName() const
{
	return name;
}

void RenderGraphNode2::setName(String name)
{
	this->name = std::move(name);
}


GraphNodeId RenderGraphDefinition2::addNode(const String& type, Vector2f pos, ConfigNode settings)
{
	const auto id = static_cast<GraphNodeId>(nodes.size());
	nodes.push_back(RenderGraphNode2(type, pos));
	nodes.back().getSettings() = settings;
	return id;
}

void RenderGraphDefinition2::load(const ConfigNode& node)
{
	nodes = node["nodes"].asVector<RenderGraphNode2>({});
}

ConfigNode RenderGraphDefinition2::toConfigNode() const
{
	ConfigNode::MapType result;
	result["nodes"] = nodes;
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

