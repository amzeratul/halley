#include "audio_object.h"
#include "audio_clip.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/core/resources/resources.h"
#include "halley/data_structures/config_node.h"
#include "halley/maths/random.h"
#include "halley/support/logger.h"
#include "sub_objects/audio_sub_object_clips.h"

using namespace Halley;

AudioObject::AudioObject()
{
	generateId();
}

AudioObject::AudioObject(const ConfigNode& node)
{
	generateId();

	group = node["group"].asString("");
	pitch = node["pitch"].asFloatRange(Range<float>(1, 1));
	volume = node["volume"].asFloatRange(Range<float>(1, 1));

	if (node.hasKey("root")) {
		root = IAudioSubObject::makeSubObject(node["root"]);
	}
}

void AudioObject::loadLegacyEvent(const ConfigNode& node)
{
	group = node["group"].asString("");
	pitch = node["pitch"].asFloatRange(Range<float>(1, 1));
	volume = node["volume"].asFloatRange(Range<float>(1, 1));

	auto clips = std::make_unique<AudioSubObjectClips>();
	clips->load(node);
	root = AudioSubObjectHandle(std::move(clips));
}

uint32_t AudioObject::getAudioObjectId() const
{
	return audioObjectId;
}

const String& AudioObject::getGroup() const
{
	return group;
}

Range<float> AudioObject::getPitch() const
{
	return pitch;
}

Range<float> AudioObject::getVolume() const
{
	return volume;
}

std::unique_ptr<AudioSource> AudioObject::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	if (root.hasValue()) {
		return root->makeSource(engine, emitter);
	} else {
		return {};
	}
}

void AudioObject::serialize(Serializer& s) const
{
	s << group;
	s << pitch;
	s << volume;
	s << root;
}

void AudioObject::deserialize(Deserializer& s)
{
	s >> group;
	s >> pitch;
	s >> volume;
	s >> root;
}

void AudioObject::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<AudioObject&>(resource));
}

std::shared_ptr<AudioObject> AudioObject::loadResource(ResourceLoader& loader)
{
	auto staticData = loader.getStatic(false);
	if (!staticData) {
		return {};
	}
	
	Deserializer s(staticData->getSpan());
	auto object = std::make_shared<AudioObject>();
	s >> *object;
	object->loadDependencies(loader.getResources());
	return object;
}

void AudioObject::loadDependencies(Resources& resources)
{
	if (root.hasValue()) {
		root->loadDependencies(resources);
	}
}

void AudioObject::generateId()
{
	static std::atomic<AudioObjectId> id = 1;
	
	audioObjectId = id++;
}
