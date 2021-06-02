#include "graphics/render_target/render_graph_definition.h"
#include "halley/bytes/byte_serializer.h"
#include "resources/resources.h"
#include "graphics/material/material_definition.h"

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
	s << method;
	s << methodParameters;
	s << position;
}

void RenderGraphDefinition::Node::deserialize(Deserializer& s)
{
	s >> id;
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
		outputPins.clear();
	} else {
		outputPins = {{ RenderGraphPinType::ColourBuffer, RenderGraphPinType::DepthStencilBuffer }};
	}

	if (material) {
		for (const auto& t: material->getTextures()) {
			inputPins.push_back(RenderGraphPinType::Texture);
		}
	}
}
