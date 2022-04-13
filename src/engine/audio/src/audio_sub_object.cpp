#include "audio_sub_object.h"

#include "audio_engine.h"
#include "audio_source_clip.h"
#include "audio_clip.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
#include "halley/bytes/byte_serializer.h"

using namespace Halley;

std::unique_ptr<IAudioSubObject> IAudioSubObject::makeSubObject(AudioSubObjectType type)
{
	switch (type) {
	case AudioSubObjectType::Clips:
		return std::make_unique<AudioSubObjectClips>();
	}
	return {};
}

std::unique_ptr<IAudioSubObject> IAudioSubObject::makeSubObject(const ConfigNode& node)
{
	const auto type = fromString<AudioSubObjectType>(node["type"].asString());
	auto obj = makeSubObject(type);
	if (obj) {
		obj->load(node);
	}
	return obj;
}

AudioSubObjectHandle::AudioSubObjectHandle(std::unique_ptr<IAudioSubObject> obj)
	: obj(std::move(obj))
{
}

IAudioSubObject& AudioSubObjectHandle::getObject()
{
	return *obj;
}

const IAudioSubObject& AudioSubObjectHandle::getObject() const
{
	return *obj;
}

IAudioSubObject& AudioSubObjectHandle::operator*()
{
	return *obj;
}

IAudioSubObject& AudioSubObjectHandle::operator*() const
{
	return *obj;
}

IAudioSubObject* AudioSubObjectHandle::operator->()
{
	return obj.get();
}

IAudioSubObject* AudioSubObjectHandle::operator->() const
{
	return obj.get();
}

bool AudioSubObjectHandle::hasValue() const
{
	return !!obj;
}

void AudioSubObjectHandle::serialize(Serializer& s) const
{
	if (obj) {
		s << static_cast<int>(obj->getType());
		obj->serialize(s);
	} else {
		s << static_cast<int>(AudioSubObjectType::None);
	}
}

void AudioSubObjectHandle::deserialize(Deserializer& s)
{
	int type;
	s >> type;
	obj = IAudioSubObject::makeSubObject(static_cast<AudioSubObjectType>(type));
	if (obj) {
		obj->deserialize(s);
	}
}

void AudioSubObjectClips::load(const ConfigNode& node)
{
	clips = node["clips"].asVector<String>({});
	delay = node["delay"].asFloat(0.0f);
	loop = node["loop"].asBool(false);
}

std::unique_ptr<AudioSource> AudioSubObjectClips::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	if (clipData.empty()) {
		return {};
	}

	auto clip = engine.getRNG().getRandomElement(clipData);
	return std::make_unique<AudioSourceClip>(clip, loop, lroundl(delay * AudioConfig::sampleRate));
}

void AudioSubObjectClips::loadDependencies(Resources& resources)
{
	if (clipData.size() != clips.size()) {
		clipData.clear();
		clipData.reserve(clips.size());

		for (auto& c: clips) {
			if (resources.exists<AudioClip>(c)) {
				clipData.push_back(resources.get<AudioClip>(c));
			} else {
				Logger::logError("AudioClip not found: \"" + c + "\".");
				clipData.push_back(std::shared_ptr<AudioClip>());
			}
		}
	}
}

void AudioSubObjectClips::serialize(Serializer& s) const
{
	s << clips;
	s << delay;
	s << loop;
}

void AudioSubObjectClips::deserialize(Deserializer& s)
{
	s >> clips;
	s >> delay;
	s >> loop;
}
