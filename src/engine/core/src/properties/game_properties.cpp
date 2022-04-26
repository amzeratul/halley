#include "properties/game_properties.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"
using namespace Halley;

GameProperties::GameProperties(const ConfigNode& node)
{
	audioProperties = AudioProperties(node["audioProperties"]);
}

ConfigNode GameProperties::toConfigNode() const
{
	ConfigNode::MapType result;
	result["audioProperties"] = audioProperties.toConfigNode();
	return result;
}

void GameProperties::serialize(Serializer& s) const
{
	s << audioProperties;
}

void GameProperties::deserialize(Deserializer& s)
{
	s >> audioProperties;
}

std::unique_ptr<GameProperties> GameProperties::loadResource(ResourceLoader& loader)
{
	auto result = std::make_unique<GameProperties>();
	Deserializer::fromBytes(*result, loader.getStatic()->getSpan());
	return result;
}

void GameProperties::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<GameProperties&>(resource));
}

const AudioProperties& GameProperties::getAudioProperties() const
{
	return audioProperties;
}

AudioProperties& GameProperties::getAudioProperties()
{
	return audioProperties;
}
