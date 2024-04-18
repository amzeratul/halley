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
	modified = false;
}

void Options::reset()
{
	options = ConfigNode::MapType();
	options["volume"] = ConfigNode::MapType();
	options["devValues"] = ConfigNode::MapType();
	options["devFlags"] = ConfigNode::MapType();
	options["keyboardLayout"] = "qwerty";
	options["resolution"] = Vector2i(1280, 720);
	modified = true;

	onReset();
}

bool Options::isModified() const
{
	return modified;
}

void Options::load(ConfigNode node)
{
	for (auto& [k, v]: node.asMap()) {
		options[k] = std::move(v);
	}
	modified = false;
}

ConfigNode Options::toConfigNode() const
{
	return ConfigNode(options);
}

void Options::setOption(std::string_view name, ConfigNode value)
{
	options[name] = std::move(value);
	modified = true;
}

ConfigNode Options::getOption(std::string_view name) const
{
	return ConfigNode(options[name]);
}

void Options::setDevValue(std::string_view name, float value)
{
	options["devValues"][name] = value;
	modified = true;
}

float Options::getDevValue(std::string_view name, float defaultValue) const
{
	return options["devValues"][name].asFloat(defaultValue);
}

void Options::setDevFlag(std::string_view name, bool value)
{
	options["devFlags"][name] = value;
	modified = true;
}

bool Options::getDevFlag(std::string_view name, bool defaultValue) const
{
	return options["devFlags"][name].asBool(defaultValue);
}

void Options::onReset()
{

}


Vector2i Options::getResolution(bool fullscreen) const
{
	return getOption("resolution" + String(fullscreen ? "_fullscreen" : "_window")).asVector2i(Vector2i(1280, 720));
}

void Options::setResolution(bool fullscreen, Vector2i resolution)
{
	setOption("resolution" + String(fullscreen ? "_fullscreen" : "_window"), ConfigNode(resolution));
}

bool Options::getFullscreen() const
{
	return false;
	// TODO
	return getOption("fullscreen").asBool(true);
}

void Options::setFullscreen(bool fullscreen)
{
	setOption("fullscreen", ConfigNode(fullscreen));
}

bool Options::getVibration() const
{
	return getOption("vibration").asBool(true);
}

void Options::setVibration(bool enabled)
{
	setOption("vibration", ConfigNode(enabled));
}

bool Options::getFlashingEffects() const
{
	return getOption("flashingEffects").asBool(true);
}

void Options::setFlashingEffects(bool enabled)
{
	setOption("flashingEffects", ConfigNode(enabled));
}

bool Options::getScreenShake() const
{
	return getOption("screenShake").asBool(true);
}

void Options::setScreenShake(bool enabled)
{
	setOption("screenShake", ConfigNode(enabled));
}

float Options::getVolume(std::string_view bus) const
{
	return options["volume"][bus].asFloat(1.0f);
}

void Options::setVolume(std::string_view bus, float value)
{
	options["volume"][bus] = value;
	modified = true;
}

void Options::applyVolumes(AudioAPI& audio)
{
	for (const auto& [k, v] : options["volume"].asMap()) {
		audio.setBusVolume(k, v.asFloat(1.0f));
	}
}

void Options::setAudioOutputType(AudioOutputType type)
{
	setOption("audioOutputType", ConfigNode(toString(type)));
}

AudioOutputType Options::getAudioOutputType() const
{
	const auto defaultValue = isPCPlatform() ? AudioOutputType::Headphones : AudioOutputType::SurroundSpeakers;
	return getOption("audioOutputType").asEnum<AudioOutputType>(defaultValue);
}

String Options::getKeyboardLayout() const
{
	return getOption("keyboardLayout").asString();
}

void Options::setKeyboardLayout(String layout)
{
	setOption("keyboardLayout", ConfigNode(std::move(layout)));
}

String Options::getLanguage() const
{
	return getOption("language").asString("en-GB");
}

void Options::setLanguage(String languageCode)
{
	setOption("language", ConfigNode(std::move(languageCode)));
}
