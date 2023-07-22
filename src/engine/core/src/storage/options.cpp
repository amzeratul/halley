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
	const auto data = saveData->getData("options");
	if (!data.empty()) {
		auto configFile = Deserializer::fromBytes<ConfigFile>(data, SerializerOptions(SerializerOptions::maxVersion));
		load(configFile.getRoot());
	} else {
		reset();
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
	volume.clear();
	devValues.clear();
	devFlags.clear();
	keyboardLayout = "qwerty";
	onReset();
}

void Options::load(const ConfigNode& node)
{
	volume = node["volume"].asMap<String, float>();
	devValues = node["devValues"].asMap<String, float>();
	devFlags = node["devFlags"].asMap<String, bool>();
	keyboardLayout = node["keyboardLayout"].asString("qwerty");
}

ConfigNode Options::toConfigNode() const
{
	auto result = ConfigNode::MapType();
	
	result["volume"] = volume;
	result["devValues"] = devValues;
	result["devFlags"] = devFlags;
	result["keyboardLayout"] = keyboardLayout;
	
	return result;
}

float Options::getVolume(std::string_view bus) const
{
	const auto iter = volume.find(bus);
	if (iter != volume.end()) {
		return iter->second;
	}
	return 1.0f;
}

void Options::setVolume(std::string_view bus, float value)
{
	volume[bus] = value;
}

void Options::applyVolumes(AudioAPI& audio)
{
	for (const auto& [k, v]: volume) {
		audio.setBusVolume(k, v);
	}
}

String Options::getKeyboardLayout() const
{
	return keyboardLayout;
}

void Options::setKeyboardLayout(String layout)
{
	keyboardLayout = std::move(layout);
}

void Options::setDevValue(std::string_view name, float value)
{
	devValues[name] = value;
}

float Options::getDevValue(std::string_view name, float defaultValue) const
{
	const auto iter = devValues.find(name);
	if (iter == devValues.end()) {
		return defaultValue;
	}
	return iter->second;
}

void Options::setDevFlag(std::string_view name, bool value)
{
	devFlags[name] = value;
}

bool Options::getDevFlag(std::string_view name, bool defaultValue) const
{
	const auto iter = devFlags.find(name);
	if (iter == devFlags.end()) {
		return defaultValue;
	}
	return iter->second;
}

void Options::onReset()
{
	
}
