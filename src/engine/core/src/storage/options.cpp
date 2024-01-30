#include "halley/storage/options.h"

#include "halley/api/audio_api.h"
#include "halley/bytes/byte_serializer.h"
using namespace Halley;

Options::Options(std::shared_ptr<ISaveData> saveData)
	: saveData(std::move(saveData))
{
	load();
}

void Options::load()
{
	reset();

	const auto data = saveData->getData("options");
	if (!data.empty()) {
		auto configFile = Deserializer::fromBytes<ConfigFile>(data, SerializerOptions(SerializerOptions::maxVersion));
		load(std::move(configFile.getRoot()));
	}
}

void Options::save()
{
	ConfigFile result;
	result.getRoot() = toConfigNode();
	auto bytes = Serializer::toBytes(result, SerializerOptions(SerializerOptions::maxVersion));
	saveData->setData("options", bytes);
}

void Options::reset()
{
	options = ConfigNode::MapType();
	options["volume"] = ConfigNode::MapType();
	options["devValues"] = ConfigNode::MapType();
	options["devFlags"] = ConfigNode::MapType();
	options["keyboardLayout"] = "qwerty";
	options["resolution"] = Vector2i(1280, 720);

	onReset();
}

void Options::load(ConfigNode node)
{
	for (auto& [k, v]: node.asMap()) {
		options[k] = std::move(v);
	}
}

ConfigNode Options::toConfigNode() const
{
	return ConfigNode(options);
}

float Options::getVolume(std::string_view bus) const
{
	return options["volume"][bus].asFloat(1.0f);
}

void Options::setVolume(std::string_view bus, float value)
{
	options["volume"][bus] = value;
}

void Options::applyVolumes(AudioAPI& audio)
{
	for (const auto& [k, v]: options["volume"].asMap()) {
		audio.setBusVolume(k, v.asFloat(1.0f));
	}
}

String Options::getKeyboardLayout() const
{
	return options["keyboardLayout"].asString();
}

void Options::setKeyboardLayout(String layout)
{
	options["keyboardLayout"] = ConfigNode(std::move(layout));
}

void Options::setDevValue(std::string_view name, float value)
{
	options["devValues"][name] = value;
}

float Options::getDevValue(std::string_view name, float defaultValue) const
{
	return options["devValues"][name].asFloat(defaultValue);
}

void Options::setDevFlag(std::string_view name, bool value)
{
	options["devFlags"][name] = value;
}

bool Options::getDevFlag(std::string_view name, bool defaultValue) const
{
	return options["devFlags"][name].asBool(defaultValue);
}

Vector2i Options::getResolution() const
{
	return options["resolution"].asVector2i();
}

void Options::setResolution(Vector2i resolution)
{
	options["resolution"] = resolution;
}

void Options::onReset()
{
	
}
