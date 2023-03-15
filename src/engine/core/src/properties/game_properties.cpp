#include "halley/properties/game_properties.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"
#include "halley/file_formats/yaml_convert.h"
using namespace Halley;

GameProperties::GameProperties(const ConfigNode& node)
{
	load(node);
}

GameProperties::GameProperties(Path path)
	: path(std::move(path))
{
	load();
}

ConfigNode GameProperties::toConfigNode() const
{
	ConfigNode::MapType result;
	result["audioProperties"] = audioProperties.toConfigNode();
	return result;
}

void GameProperties::save() const
{
	YAMLConvert::EmitOptions options;
	auto config = YAMLConvert::generateYAML(toConfigNode(), options);
	Path::writeFile(path, config);
}

void GameProperties::load()
{
	auto data = Path::readFile(path);
	if (!data.empty()) {
		load(YAMLConvert::parseConfig(data).getRoot());
	}
}

void GameProperties::load(const ConfigNode& node)
{
	audioProperties = AudioProperties(node["audioProperties"]);
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
