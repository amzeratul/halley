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
#include "behaviours/audio_voice_dynamics_behaviour.h"
#include "halley/support/logger.h"

using namespace Halley;

AudioEvent::AudioEvent() = default;

AudioEvent::AudioEvent(const ConfigNode& config)
{
	if (config.hasKey("actions")) {
		for (auto& actionNode: config["actions"]) {
			const auto type = fromString<AudioEventActionType>(actionNode["type"].asString());
			if (type == AudioEventActionType::Play) {
				actions.push_back(std::make_unique<AudioEventActionPlay>(actionNode));
			}
		}
	}
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
	uint32_t size;
	s >> size;
	for (uint32_t i = 0; i < size; ++i) {
		String name;
		s >> name;
		auto type = fromString<AudioEventActionType>(name);
		std::unique_ptr<IAudioEventAction> newAction;

		switch (type) {
		case AudioEventActionType::Play:
			newAction = std::make_unique<AudioEventActionPlay>();
			break;
		case AudioEventActionType::Stop:
			newAction = std::make_unique<AudioEventActionStop>();
			break;
		case AudioEventActionType::Pause:
			newAction = std::make_unique<AudioEventActionPause>();
			break;
		case AudioEventActionType::Resume:
			newAction = std::make_unique<AudioEventActionResume>();
			break;
		case AudioEventActionType::SetVolume:
			newAction = std::make_unique<AudioEventActionSetVolume>();
			break;
		case AudioEventActionType::SetSwitch:
			newAction = std::make_unique<AudioEventActionSetSwitch>();
			break;
		case AudioEventActionType::SetVariable:
			newAction = std::make_unique<AudioEventActionSetVariable>();
			break;
		}

		if (newAction) {
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

void AudioEvent::loadDependencies(Resources& resources)
{
	for (auto& a: actions) {
		a->loadDependencies(resources);
	}
}



AudioEventActionPlay::AudioEventActionPlay(const ConfigNode& node)
{
	if (node.hasKey("object")) {
		objectName = node["object"].asString();
		legacy = false;
	} else {
		objectName = "";
		auto obj = std::make_shared<AudioObject>();
		obj->loadLegacyEvent(node);
		object = obj;
		legacy = true;
	}
}

bool AudioEventActionPlay::run(AudioEngine& engine, AudioEventId uniqueId, AudioEmitter& emitter) const
{
	if (!object) {
		return false;
	}

	const uint32_t audioObjectId = object->getAudioObjectId();

	const auto gainRange = object->getVolume();
	const float gain = engine.getRNG().getFloat(gainRange.start, gainRange.end);
	const auto pitchRange = object->getPitch();
	const float pitch = clamp(engine.getRNG().getFloat(pitchRange.start, pitchRange.end), 0.1f, 2.0f);

	auto source = object->makeSource(engine, emitter);
	auto voice = std::make_unique<AudioVoice>(engine, std::move(source), gain, pitch, engine.getGroupId(object->getGroup()));
	voice->setIds(uniqueId, audioObjectId);
	
	emitter.addVoice(std::move(voice));
	return true;
}

void AudioEventActionPlay::serialize(Serializer& s) const
{
	s << legacy;
	if (legacy) {
		s << *object;
	} else {
		s << objectName;
	}
}

void AudioEventActionPlay::deserialize(Deserializer& s)
{
	s >> legacy;
	if (legacy) {
		auto obj = std::make_shared<AudioObject>();
		s >> *obj;
		object = obj;
	} else {
		s >> objectName;
	}
}

void AudioEventActionPlay::loadDependencies(Resources& resources)
{
	if (legacy) {
		const_cast<AudioObject&>(*object).loadDependencies(resources); // :|
	} else {
		if (!objectName.isEmpty() && (!object || object->getAssetId() != objectName)) {
			object = resources.get<AudioObject>(objectName);
		}
	}
}

AudioEventActionObject::AudioEventActionObject(const ConfigNode& node)
{
	objectName = node["object"].asString();
}

void AudioEventActionObject::serialize(Serializer& s) const
{
	s << objectName;
}

void AudioEventActionObject::deserialize(Deserializer& s)
{
	s >> objectName;
}

void AudioEventActionObject::loadDependencies(Resources& resources)
{
	if (!objectName.isEmpty() && (!object || object->getAssetId() != objectName)) {
		object = resources.get<AudioObject>(objectName);
	}
}

AudioEventActionStop::AudioEventActionStop(const ConfigNode& config)
	: AudioEventActionObject(config)
{	
}

bool AudioEventActionStop::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	if (!object) {
		return false;
	}
	
	const AudioObjectId audioObjectId = object->getAudioObjectId();

	emitter.forVoices(audioObjectId, [&] (AudioVoice& voice)
	{
		voice.stop();
	});

	return true;
}

AudioEventActionPause::AudioEventActionPause(const ConfigNode& config)
	: AudioEventActionObject(config)
{}

bool AudioEventActionPause::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	if (!object) {
		return false;
	}
	
	const AudioObjectId audioObjectId = object->getAudioObjectId();

	emitter.forVoices(audioObjectId, [&] (AudioVoice& voice)
	{
		voice.pause();
	});

	return true;
}

AudioEventActionResume::AudioEventActionResume(const ConfigNode& config)
	: AudioEventActionObject(config)
{}

bool AudioEventActionResume::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	if (!object) {
		return false;
	}
	
	const AudioObjectId audioObjectId = object->getAudioObjectId();

	emitter.forVoices(audioObjectId, [&] (AudioVoice& voice)
	{
		voice.resume();
	});

	return true;
}

AudioEventActionSetVolume::AudioEventActionSetVolume(const ConfigNode& config)
	: AudioEventActionObject(config)
{
	gain = config["gain"].asFloat(1.0f);
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

AudioEventActionSetSwitch::AudioEventActionSetSwitch(const ConfigNode& config)
{
	switchId = config["switchId"].asString();
	value = config["value"].asString();
}

bool AudioEventActionSetSwitch::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	// TODO

	return true;
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

AudioEventActionSetVariable::AudioEventActionSetVariable(const ConfigNode& config)
{
	variableId = config["variableId"].asString();
	value = config["value"].asFloat();
}

bool AudioEventActionSetVariable::run(AudioEngine& engine, AudioEventId id, AudioEmitter& emitter) const
{
	// TODO

	return true;
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
