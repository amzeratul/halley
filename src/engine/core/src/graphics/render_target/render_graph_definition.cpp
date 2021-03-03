#include "graphics/render_target/render_graph_definition.h"
#include "halley/bytes/byte_serializer.h"

using namespace Halley;

RenderGraphDefinition::RenderGraphDefinition()
{
}

RenderGraphDefinition::RenderGraphDefinition(const ConfigNode& config)
{
	// TODO
}

std::unique_ptr<RenderGraphDefinition> RenderGraphDefinition::loadResource(ResourceLoader& loader)
{
	auto result = std::make_unique<RenderGraphDefinition>();
	Deserializer::fromBytes(*result, loader.getStatic()->getSpan());
	return result;
}

void RenderGraphDefinition::serialize(Serializer& s) const
{
	// TODO
}

void RenderGraphDefinition::deserialize(Deserializer& s)
{
	// TODO
}
