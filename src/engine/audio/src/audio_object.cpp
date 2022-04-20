#include "audio_object.h"
#include "audio_clip.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/core/resources/resources.h"
#include "halley/data_structures/config_node.h"
#include "halley/file_formats/yaml_convert.h"
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
	objects = node["objects"].asVector<AudioSubObjectHandle>({});
}

ConfigNode AudioObject::toConfigNode() const
{
	ConfigNode::MapType result;

	if (!group.isEmpty()) {
		result["group"] = group;
	}
	if (pitch != Range<float>(1, 1)) {
		result["pitch"] = pitch;
	}
	if (volume != Range<float>(1, 1)) {
		result["volume"] = volume;
	}
	result["objects"] = objects;
	
	return result;
}

void AudioObject::loadLegacyEvent(const ConfigNode& node)
{
	group = node["group"].asString("");
	pitch = node["pitch"].asFloatRange(Range<float>(1, 1));
	volume = node["volume"].asFloatRange(Range<float>(1, 1));

	auto clips = std::make_unique<AudioSubObjectClips>();
	clips->load(node);

	objects.clear();
	objects.emplace_back(AudioSubObjectHandle(std::move(clips)));
}

void AudioObject::legacyToConfigNode(ConfigNode& result) const
{
	if (!group.isEmpty()) {
		result["group"] = group;
	}
	if (pitch != Range<float>(1, 1)) {
		result["pitch"] = pitch;
	}
	if (volume != Range<float>(1, 1)) {
		result["volume"] = volume;
	}
	dynamic_cast<const AudioSubObjectClips&>(objects[0].getObject()).toLegacyConfigNode(result);
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

gsl::span<AudioSubObjectHandle> AudioObject::getSubObjects()
{
	return objects;
}

std::unique_ptr<AudioSource> AudioObject::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	if (!objects.empty()) {
		return objects[0]->makeSource(engine, emitter);
	} else {
		return {};
	}
}

void AudioObject::serialize(Serializer& s) const
{
	s << group;
	s << pitch;
	s << volume;
	s << objects;
}

void AudioObject::deserialize(Deserializer& s)
{
	s >> group;
	s >> pitch;
	s >> volume;
	s >> objects;
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
	if (!objects.empty()) {
		objects[0]->loadDependencies(resources);
	}
}

void AudioObject::makeDefault()
{
}

String AudioObject::toYAML() const
{
	YAMLConvert::EmitOptions options;
	return YAMLConvert::generateYAML(toConfigNode(), options);
}

void AudioObject::generateId()
{
	static std::atomic<AudioObjectId> id = 1;
	
	audioObjectId = id++;
}
