#include "halley/audio/audio_object.h"

#include "halley/audio/audio_attenuation.h"
#include "halley/audio/audio_clip.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/resources/resources.h"
#include "halley/data_structures/config_node.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/maths/random.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "halley/audio/sub_objects/audio_sub_object_clips.h"

using namespace Halley;

AudioObject::AudioObject()
	: pitch(1, 1)
	, gain(1, 1)
{
	generateId();
}

AudioObject::AudioObject(const ConfigNode& node)
{
	generateId();

	bus = node[node.hasKey("bus") ? "bus" : "group"].asString("");
	pitch = node["pitch"].asFloatRange(Range<float>(1, 1));
	gain = node["gain"].asFloatRange(Range<float>(1, 1));
	dopplerScale = node["dopplerScale"].asFloat(0.0f);
	objects = node["objects"].asVector<AudioSubObjectHandle>({});
	attenuationOverride = node["attenuationOverride"].asOptional<AudioAttenuation>();
	pruneDistant = node["pruneDistant"].asBool(true);
	cooldown = node["cooldown"].asOptional<float>();
	maxInstances = node["maxInstances"].asOptional<int>();
	priority = node["priority"].asInt(0);
}

ConfigNode AudioObject::toConfigNode() const
{
	ConfigNode::MapType result;

	if (!bus.isEmpty()) {
		result["bus"] = bus;
	}
	if (pitch != Range<float>(1, 1)) {
		result["pitch"] = pitch;
	}
	if (gain != Range<float>(1, 1)) {
		result["gain"] = gain;
	}
	if (std::abs(dopplerScale) > 0.0001f) {
		result["dopplerScale"] = dopplerScale;
	}
	if (attenuationOverride) {
		result["attenuationOverride"] = *attenuationOverride;
	}
	result["pruneDistant"] = pruneDistant;
	if (cooldown) {
		result["cooldown"] = cooldown;
	}
	if (maxInstances) {
		result["maxInstances"] = maxInstances;
	}
	if (priority) {
		result["priority"] = priority;
	}
	result["objects"] = objects;
	
	return result;
}

void AudioObject::loadLegacyEvent(const ConfigNode& node)
{
	bus = node["group"].asString("");
	pitch = node["pitch"].asFloatRange(Range<float>(1, 1));
	if (node.hasKey("volume")) {
		gain = node["volume"].asFloatRange(Range<float>(1, 1));
	} else {
		gain = node["gain"].asFloatRange(Range<float>(1, 1));
	}
	dopplerScale = node["dopplerScale"].asFloat(0.0f);

	auto clips = std::make_unique<AudioSubObjectClips>();
	clips->load(node);

	objects.clear();
	objects.emplace_back(AudioSubObjectHandle(std::move(clips)));
}

void AudioObject::legacyToConfigNode(ConfigNode& result) const
{
	if (!bus.isEmpty()) {
		result["bus"] = bus;
	}
	if (pitch != Range<float>(1, 1)) {
		result["pitch"] = pitch;
	}
	if (gain != Range<float>(1, 1)) {
		result["gain"] = gain;
	}
	dynamic_cast<const AudioSubObjectClips&>(objects[0].getObject()).toLegacyConfigNode(result);
}

uint32_t AudioObject::getAudioObjectId() const
{
	return audioObjectId;
}

const String& AudioObject::getBus() const
{
	return bus;
}

Range<float> AudioObject::getPitch() const
{
	return pitch;
}

Range<float> AudioObject::getGain() const
{
	return gain;
}

Range<float>& AudioObject::getPitch()
{
	return pitch;
}

Range<float>& AudioObject::getGain()
{
	return gain;
}

float AudioObject::getDopplerScale() const
{
	return dopplerScale;
}

void AudioObject::setDopplerScale(float scale)
{
	dopplerScale = scale;
}

std::optional<AudioAttenuation> AudioObject::getAttenuationOverride() const
{
	return attenuationOverride;
}

AudioAttenuation& AudioObject::getMutableAttenuationOverride()
{
	if (!attenuationOverride) {
		attenuationOverride = AudioAttenuation();
	}
	return *attenuationOverride;
}

void AudioObject::setAttenuationOverride(std::optional<AudioAttenuation> value)
{
	this->attenuationOverride = value;
}

bool AudioObject::getPruneDistant() const
{
	return pruneDistant;
}

void AudioObject::setPruneDistant(bool value)
{
	pruneDistant = value;
}

std::optional<float> AudioObject::getCooldown() const
{
	return cooldown;
}

void AudioObject::setCooldown(std::optional<float> value)
{
	cooldown = value;
}

std::optional<int> AudioObject::getMaxInstances() const
{
	return maxInstances;
}

void AudioObject::setMaxInstances(std::optional<int> maxInstances)
{
	this->maxInstances = maxInstances;
}

int AudioObject::getPriority() const
{
	return priority;
}

void AudioObject::setPriority(int value)
{
	priority = value;
}

void AudioObject::setBus(String bus)
{
	this->bus = std::move(bus);
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
	s << bus;
	s << pitch;
	s << gain;
	s << dopplerScale;
	s << objects;
	s << attenuationOverride;
	s << pruneDistant;
	s << cooldown;
	s << maxInstances;
	s << priority;
}

void AudioObject::deserialize(Deserializer& s)
{
	s >> bus;
	s >> pitch;
	s >> gain;
	s >> dopplerScale;
	s >> objects;
	s >> attenuationOverride;
	s >> pruneDistant;
	s >> cooldown;
	s >> maxInstances;
	s >> priority;
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

AudioSubObjectType AudioObject::getType() const
{
	return AudioSubObjectType::None;
}

size_t AudioObject::getNumSubObjects() const
{
	return objects.size();
}

AudioSubObjectHandle& AudioObject::getSubObject(size_t n)
{
	return objects.at(n);
}

bool AudioObject::canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const
{
	return !caseName.has_value();
}

void AudioObject::addObject(AudioSubObjectHandle object, const std::optional<String>& caseName, size_t idx)
{
	objects.insert(objects.begin() + std::min(objects.size(), idx), std::move(object));
}

AudioSubObjectHandle AudioObject::removeObject(const IAudioObject* object)
{
	const auto iter = std_ex::find_if(objects, [&] (const auto& o) { return &o.getObject() == object; });
	if (iter != objects.end()) {
		auto handle = std::move(*iter);
		objects.erase(iter);
		return handle;
	}
	return AudioSubObjectHandle();
}

void AudioObject::generateId()
{
	static std::atomic<AudioObjectId> id = 1;
	
	audioObjectId = id++;
}
