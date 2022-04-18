#include "audio_event.h"
#include "audio_engine.h"
#include "audio_clip.h"
#include "halley/resources/resource_data.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/config_file.h"
#include "halley/text/string_converter.h"
#include "halley/core/resources/resources.h"
#include "audio_filter_resample.h"
#include "audio_object.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/support/logger.h"

using namespace Halley;

AudioEvent::AudioEvent() = default;

AudioEvent::AudioEvent(const AudioEvent& other)
{
	*this = other;
}

AudioEvent& AudioEvent::operator=(const AudioEvent& other)
{
	// eh, will do
	auto bytes = Serializer::toBytes(other);
	auto s = Deserializer(bytes);
	deserialize(s);
	setAssetId(other.getAssetId());
	return *this;
}

AudioEvent::AudioEvent(const ConfigNode& config)
{
	if (config.hasKey("actions")) {
		for (auto& actionNode: config["actions"]) {
			const auto type = fromString<AudioEventActionType>(actionNode["type"].asString());
			if (auto action = makeAction(type); action) {
				action->load(actionNode);
				actions.push_back(std::move(action));
			}
		}
	}
}

String AudioEvent::toYAML() const
{
	auto actionsNode = ConfigNode::SequenceType();
	for (const auto& action: actions) {
		actionsNode.push_back(action->toConfigNode());
	}
	
	ConfigNode result = ConfigNode::MapType();
	result["actions"] = std::move(actionsNode);

	YAMLConvert::EmitOptions options;
	return YAMLConvert::generateYAML(result, options);
}

size_t AudioEvent::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	size_t nEmitters = 0;
	for (const auto& a: actions) {
		if (a->run(engine, id, emitter)) {
			++nEmitters;
		}
	}
	return nEmitters;
}

const Vector<std::unique_ptr<IAudioEventAction>>& AudioEvent::getActions() const
{
	return actions;
}

Vector<std::unique_ptr<IAudioEventAction>>& AudioEvent::getActions()
{
	return actions;
}

void AudioEvent::serialize(Serializer& s) const
{
	s << uint32_t(actions.size());
	for (auto& a: actions) {
		s << toString(a->getType());
		s << *a;
	}
}

void AudioEvent::deserialize(Deserializer& s)
{
	actions.clear();
	
	uint32_t size;
	s >> size;
	for (uint32_t i = 0; i < size; ++i) {
		String name;
		s >> name;
		auto type = fromString<AudioEventActionType>(name);

		if (std::unique_ptr<IAudioEventAction> newAction = makeAction(type); newAction) {
			s >> *newAction;
			actions.push_back(std::move(newAction));
		}
	}
}

void AudioEvent::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<AudioEvent&>(resource));
}

std::shared_ptr<AudioEvent> AudioEvent::loadResource(ResourceLoader& loader)
{
	auto staticData = loader.getStatic(false);
	if (!staticData) {
		return {};
	}
	
	Deserializer s(staticData->getSpan());
	auto event = std::make_shared<AudioEvent>();
	s >> *event;
	event->loadDependencies(loader.getResources());
	return event;
}

void AudioEvent::makeDefault()
{
}

void AudioEvent::loadDependencies(Resources& resources)
{
	for (auto& a: actions) {
		a->loadDependencies(resources);
	}
}

std::unique_ptr<IAudioEventAction> AudioEvent::makeAction(AudioEventActionType type)
{
	switch (type) {
	case AudioEventActionType::Play:
		return std::make_unique<AudioEventActionPlay>();
	case AudioEventActionType::Stop:
		return std::make_unique<AudioEventActionStop>();
	case AudioEventActionType::Pause:
		return std::make_unique<AudioEventActionPause>();
	case AudioEventActionType::Resume:
		return std::make_unique<AudioEventActionResume>();
	case AudioEventActionType::SetVolume:
		return std::make_unique<AudioEventActionSetVolume>();
	case AudioEventActionType::SetSwitch:
		return std::make_unique<AudioEventActionSetSwitch>();
	case AudioEventActionType::SetVariable:
		return std::make_unique<AudioEventActionSetVariable>();
	}
	return {};
}

String AudioEvent::getActionName(AudioEventActionType type)
{
	switch (type) {
	case AudioEventActionType::Play:
		return "Play";
	case AudioEventActionType::Stop:
		return "Stop";
	case AudioEventActionType::Pause:
		return "Pause";
	case AudioEventActionType::Resume:
		return "Resume";
	case AudioEventActionType::SetSwitch:
		return "Set Switch";
	case AudioEventActionType::SetVariable:
		return "Set Variable";
	case AudioEventActionType::SetVolume:
		return "Set Volume";
	}
	return "Unknown";
}


ConfigNode IAudioEventAction::toConfigNode() const
{
	ConfigNode::MapType result;
	result["type"] = toString(getType());
	return result;
}

void AudioEventActionObject::loadObject(const ConfigNode& node, bool loadObject)
{
	if (loadObject) {
		objectName = node["object"].asString();
	}
	if (node.hasKey("fade")) {
		fade = AudioFade(node["fade"]);
	}
}

void AudioEventActionObject::serialize(Serializer& s) const
{
	s << objectName;
	s << fade;
}

void AudioEventActionObject::deserialize(Deserializer& s)
{
	s >> objectName;
	s >> fade;
}

void AudioEventActionObject::loadDependencies(Resources& resources)
{
	if (!objectName.isEmpty() && (!object || object->getAssetId() != objectName)) {
		object = resources.get<AudioObject>(objectName);
	}
}

ConfigNode AudioEventActionObject::toConfigNode() const
{
	auto result = IAudioEventAction::toConfigNode();

	if (!objectName.isEmpty()) {
		result["object"] = objectName;
	}
	if (fade.hasFade()) {
		result["fade"] = fade.toConfigNode();
	}

	return result;
}

const String& AudioEventActionObject::getObjectName() const
{
	return objectName;
}

void AudioEventActionObject::setObjectName(const String& name, Resources& resources)
{
	objectName = name;
	loadDependencies(resources);
}

const AudioFade& AudioEventActionObject::getFade() const
{
	return fade;
}

AudioFade& AudioEventActionObject::getFade()
{
	return fade;
}


void AudioEventActionPlay::load(const ConfigNode& node)
{
	loadObject(node, false);
	legacy = !node.hasKey("object");
	
	if (legacy) {
		objectName = "";
		auto obj = std::make_shared<AudioObject>();
		obj->loadLegacyEvent(node);
		object = obj;
		playGain = Range<float>(1, 1);
	} else {
		objectName = node["object"].asString();
		playGain = node["gain"].asFloatRange(Range<float>(1, 1));
	}

	delay = node["delay"].asFloat(0);
}

bool AudioEventActionPlay::run(AudioEngine& engine, AudioEventId uniqueId, AudioEmitter& emitter) const
{
	if (!object) {
		return false;
	}

	const uint32_t audioObjectId = object->getAudioObjectId();

	const auto gainRange = object->getVolume() * playGain;
	const float gain = engine.getRNG().getFloat(gainRange.start, gainRange.end);
	const auto pitchRange = object->getPitch();
	const float pitch = clamp(engine.getRNG().getFloat(pitchRange.start, pitchRange.end), 0.1f, 2.0f);
	const auto delaySamples = std::lroundf(delay * static_cast<float>(AudioConfig::sampleRate));
	
	auto source = object->makeSource(engine, emitter);
	auto voice = std::make_unique<AudioVoice>(engine, std::move(source), gain, pitch, delaySamples, engine.getGroupId(object->getGroup()));
	voice->setIds(uniqueId, audioObjectId);
	voice->play(fade);
	
	emitter.addVoice(std::move(voice));
	return true;
}

float AudioEventActionPlay::getDelay() const
{
	return delay;
}

void AudioEventActionPlay::setDelay(float delay)
{
	this->delay = delay;
}

Range<float> AudioEventActionPlay::getGain() const
{
	return playGain;
}

Range<float>& AudioEventActionPlay::getGain()
{
	return playGain;
}

void AudioEventActionPlay::setGain(Range<float> gain)
{
	playGain = gain;
}

void AudioEventActionPlay::serialize(Serializer& s) const
{
	s << legacy;
	if (legacy) {
		s << *object;
	} else {
		AudioEventActionObject::serialize(s);
	}
	s << playGain;
	s << delay;
}

void AudioEventActionPlay::deserialize(Deserializer& s)
{
	s >> legacy;
	if (legacy) {
		auto obj = std::make_shared<AudioObject>();
		s >> *obj;
		object = obj;
	} else {
		AudioEventActionObject::deserialize(s);
	}
	s >> playGain;
	s >> delay;
}

void AudioEventActionPlay::loadDependencies(Resources& resources)
{
	if (legacy) {
		const_cast<AudioObject&>(*object).loadDependencies(resources); // :|
	} else {
		AudioEventActionObject::loadDependencies(resources);
	}
}

ConfigNode AudioEventActionPlay::toConfigNode() const
{
	auto result = AudioEventActionObject::toConfigNode();

	if (std::abs(playGain.start - 1.0f) > 0.0001f && std::abs(playGain.end - 1.0f) > 0.0001f) {
		result["playGain"] = playGain;
	}
	if (delay > 0.0001f) {
		result["delay"] = delay;
	}
	
	return result;
}

void AudioEventActionStop::load(const ConfigNode& config)
{
	loadObject(config);
}

bool AudioEventActionStop::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	if (!object) {
		return false;
	}
	
	const AudioObjectId audioObjectId = object->getAudioObjectId();

	emitter.forVoices(audioObjectId, [&] (AudioVoice& voice)
	{
		voice.stop(fade);
	});

	return true;
}

void AudioEventActionPause::load(const ConfigNode& config)
{
	loadObject(config);
}

bool AudioEventActionPause::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	if (!object) {
		return false;
	}
	
	const AudioObjectId audioObjectId = object->getAudioObjectId();

	emitter.forVoices(audioObjectId, [&] (AudioVoice& voice)
	{
		voice.pause(fade);
	});

	return true;
}

void AudioEventActionResume::load(const ConfigNode& config)
{
	loadObject(config);
}

bool AudioEventActionResume::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	if (!object) {
		return false;
	}
	
	const AudioObjectId audioObjectId = object->getAudioObjectId();

	emitter.forVoices(audioObjectId, [&] (AudioVoice& voice)
	{
		voice.resume(fade);
	});

	return true;
}

void AudioEventActionSetVolume::load(const ConfigNode& config)
{
	loadObject(config);
	gain = config["gain"].asFloat(1.0f);
}

ConfigNode AudioEventActionSetVolume::toConfigNode() const
{
	auto result = AudioEventActionObject::toConfigNode();
	result["gain"] = gain;
	return result;
}

bool AudioEventActionSetVolume::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	if (!object) {
		return false;
	}
	
	const AudioObjectId audioObjectId = object->getAudioObjectId();

	emitter.forVoices(audioObjectId, [&] (AudioVoice& voice)
	{
		voice.setUserGain(gain);
	});

	return true;
}

float AudioEventActionSetVolume::getGain() const
{
	return gain;
}

void AudioEventActionSetVolume::setGain(float value)
{
	gain = value;
}

void AudioEventActionSetVolume::serialize(Serializer& s) const
{
	AudioEventActionObject::serialize(s);
	s << gain;
}

void AudioEventActionSetVolume::deserialize(Deserializer& s)
{
	AudioEventActionObject::deserialize(s);
	s >> gain;
}

void AudioEventActionSetSwitch::load(const ConfigNode& config)
{
	switchId = config["switchId"].asString();
	value = config["value"].asString();
}

ConfigNode AudioEventActionSetSwitch::toConfigNode() const
{
	auto result = IAudioEventAction::toConfigNode();
	result["switchId"] = switchId;
	result["value"] = value;
	return result;
}

bool AudioEventActionSetSwitch::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	emitter.setSwitchValue(switchId, value);

	return true;
}

const String& AudioEventActionSetSwitch::getSwitchId() const
{
	return switchId;
}

const String& AudioEventActionSetSwitch::getValue() const
{
	return value;
}

void AudioEventActionSetSwitch::setSwitchId(String id)
{
	switchId = std::move(id);
}

void AudioEventActionSetSwitch::setValue(String val)
{
	value = std::move(val);
}

void AudioEventActionSetSwitch::serialize(Serializer& s) const
{
	s << switchId;
	s << value;
}

void AudioEventActionSetSwitch::deserialize(Deserializer& s)
{
	s >> switchId;
	s >> value;
}

void AudioEventActionSetVariable::load(const ConfigNode& config)
{
	variableId = config["variableId"].asString();
	value = config["value"].asFloat();
}

ConfigNode AudioEventActionSetVariable::toConfigNode() const
{
	auto result = IAudioEventAction::toConfigNode();
	result["variableId"] = variableId;
	result["value"] = value;
	return result;
}

bool AudioEventActionSetVariable::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	emitter.setVariableValue(variableId, value);

	return true;
}

const String& AudioEventActionSetVariable::getVariableId() const
{
	return variableId;
}

float AudioEventActionSetVariable::getValue() const
{
	return value;
}

void AudioEventActionSetVariable::setVariableId(String id)
{
	variableId = std::move(id);
}

void AudioEventActionSetVariable::setValue(float val)
{
	value = val;
}

void AudioEventActionSetVariable::serialize(Serializer& s) const
{
	s << variableId;
	s << value;
}

void AudioEventActionSetVariable::deserialize(Deserializer& s)
{
	s >> variableId;
	s >> value;
}
