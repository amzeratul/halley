#include "halley/storage/options.h"

#include "halley/api/audio_api.h"
#include "halley/api/halley_api.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/input/control_bindings.h"
using namespace Halley;

Options::Options(const HalleyAPI& api)
{
	localContainer = api.system->getStorageContainer(SaveDataType::SaveLocal);
	if (api.platform->canProvideCloudSave()) {
		roamingContainer = api.platform->getCloudSaveContainer();
	} else {
		roamingContainer = api.system->getStorageContainer(SaveDataType::SaveRoaming);
	}

	load();
}

void Options::load()
{
	reset();

	waitingForRoaming = true;
	loadLocal();
	loadRoaming();
	modified = false;
}

void Options::loadLocal()
{
	const auto data = localContainer->getData("options");
	if (!data.empty()) {
		auto configFile = Deserializer::fromBytes<ConfigFile>(data, SerializerOptions(SerializerOptions::maxVersion));
		load(std::move(configFile.getRoot()), false);
	}
}

void Options::loadRoaming()
{
	if (roamingContainer->isReady()) {
		const auto data = roamingContainer->getData("options_roaming");
		if (!data.empty()) {
			auto configFile = Deserializer::fromBytes<ConfigFile>(data, SerializerOptions(SerializerOptions::maxVersion));
			load(std::move(configFile.getRoot()), true);
		}
		waitingForRoaming = false;
	}
}

void Options::load(ConfigNode node, bool roaming)
{
	for (auto& [k, v]: node.asMap()) {
		options[k] = std::move(v);
	}
}

void Options::save()
{
	{
		ConfigFile result;
		result.getRoot() = toConfigNode(false);
		auto bytes = Serializer::toBytes(result, SerializerOptions(SerializerOptions::maxVersion));
		localContainer->setData("options", bytes);
	}

	{
		ConfigFile result;
		result.getRoot() = toConfigNode(true);
		auto bytes = Serializer::toBytes(result, SerializerOptions(SerializerOptions::maxVersion));
		roamingContainer->setData("options_roaming", bytes);
	}

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
	markModified();

	onReset();
}

void Options::update(Time t)
{
	if (waitingForRoaming) {
		loadRoaming();
	}
	
	if (saveCooldown > 0) {
		saveCooldown -= t;
	} else if (isModified()) {
		save();
		saveCooldown = 2.0;
	}
}

bool Options::isModified() const
{
	return modified;
}

void Options::markModified()
{
	modified = true;
}

ConfigNode Options::toConfigNode(bool roaming) const
{
	ConfigNode result;
	for (const auto& [k, v]: options.asMap()) {
		if (isRoamingKey(k) == roaming) {
			result[k] = ConfigNode(v);
		}
	}
	return std::move(result);
}

bool Options::isRoamingKey(const String& key) const
{
	return key == "language" || key == "flashingEffects" || key == "screenShake";
}

void Options::setOption(std::string_view name, ConfigNode value)
{
	if (options[name] != value) {
		options[name] = std::move(value);
		markModified();
	}
}

ConfigNode Options::getOption(std::string_view name) const
{
	return ConfigNode(options[name]);
}

bool Options::hasOption(std::string_view name) const
{
	return options.hasKey(name);
}

void Options::setDevValue(std::string_view name, float value)
{
	options["devValues"][name] = value;
	markModified();
}

float Options::getDevValue(std::string_view name, float defaultValue) const
{
	return options["devValues"][name].asFloat(defaultValue);
}

void Options::setDevFlag(std::string_view name, bool value)
{
	options["devFlags"][name] = value;
	markModified();
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
	if constexpr (isPCPlatform()) {
	#ifdef DEV_BUILD
		const bool defaultValue = false;
	#else
		const bool defaultValue = true;
	#endif

		return getOption("fullscreen").asBool(defaultValue);
	} else {
		return true;
	}
}

void Options::setFullscreen(bool fullscreen)
{
	setOption("fullscreen", ConfigNode(fullscreen));
}

void Options::setUISize(Vector2f uiSize)
{
	setOption("uiSize", ConfigNode(uiSize));
}

Vector2f Options::getUISize() const
{
	return getOption("uiSize").asVector2f(Vector2f(1, 1));
}

Vector2f Options::getMaxOverscan(Vector2i resolution) const
{
	return Vector2f(0.1f, 0.1f); // TODO
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

void Options::setAudioEventLogging(std::optional<LoggerLevel> level, const std::optional<String>& prefix)
{
	setOption("audio_log_level", ConfigNode(level));
	setOption("audio_log_prefix", ConfigNode(prefix));
	save();
}

void Options::loadAudioEventLogging(AudioAPI& audioAPI) const
{
	audioAPI.setEventLogging(getOption("audio_log_level").asOptional<LoggerLevel>(), getOption("audio_log_prefix").asOptional<String>());
}

void Options::loadControlBindings(ControlBindingConfigs config, bool devMode)
{
	controlBindings = std::make_shared<ControlBindings>(config, devMode);
	controlBindings->load(getOption("control_bindings"));
}

ControlBindings& Options::getControlBindings() const
{
	if (!controlBindings) {
		throw Exception("Control bindings not set", HalleyExceptions::Utils);
	}
	return *controlBindings;
}

void Options::saveControlBindings()
{
	if (controlBindings) {
		setOption("control_bindings", controlBindings->toConfigNode());
	}
}

float Options::getVolume(std::string_view bus) const
{
	return options["volume"][bus].asFloat(1.0f);
}

void Options::setVolume(std::string_view bus, float value)
{
	options["volume"][bus] = value;
	markModified();
}

void Options::applyVolumes(AudioAPI& audio)
{
	for (const auto& [k, v] : options["volume"].asMap()) {
		audio.setBusVolume(k, v.asFloat(1.0f), {}, "options");
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

I18NLanguage Options::getLanguage() const
{
	return I18NLanguage(getOption("language").asString("en-GB"));
}

bool Options::isLanguageSet() const
{
	return hasOption("language");
}

void Options::setLanguage(I18NLanguage languageCode)
{
	setOption("language", languageCode.toConfigNode());
}
