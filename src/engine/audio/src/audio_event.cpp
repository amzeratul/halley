#include "audio_event.h"
#include "audio_engine.h"
#include "audio_clip.h"
#include "halley/resources/resource_data.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/config_file.h"
#include "halley/text/string_converter.h"
#include "halley/core/resources/resources.h"
#include "audio_source_clip.h"
#include "audio_filter_resample.h"
#include "audio_object.h"
#include "audio_source_object.h"
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

size_t AudioEvent::run(AudioEngine& engine, uint32_t id, const AudioPosition& position) const
{
	size_t nEmitters = 0;
	for (const auto& a: actions) {
		if (a->run(engine, id, position)) {
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
		if (type == AudioEventActionType::Play) {
			auto newAction = std::make_unique<AudioEventActionPlay>();
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



AudioEventActionPlay::AudioEventActionPlay() = default;

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

bool AudioEventActionPlay::run(AudioEngine& engine, uint32_t id, const AudioPosition& position) const
{
	if (!object) {
		return false;
	}

	const auto gainRange = object->getVolume();
	const float gain = engine.getRNG().getFloat(gainRange.start, gainRange.end);
	const auto pitchRange = object->getPitch();
	const float pitch = clamp(engine.getRNG().getFloat(pitchRange.start, pitchRange.end), 0.1f, 2.0f);

	auto source = std::make_shared<AudioSourceObject>(engine, object);
	auto voice = std::make_unique<AudioVoice>(engine, source, position, gain, pitch, engine.getGroupId(object->getGroup()));
	engine.addEmitter(id, std::move(voice));
	return true;
}

AudioEventActionType AudioEventActionPlay::getType() const
{
	return AudioEventActionType::Play;
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
