#include "halley/audio/sub_objects/audio_sub_object_switch.h"

#include "../audio_emitter.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/properties/audio_properties.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

AudioSubObjectSwitch::AudioSubObjectSwitch(const ConfigNode& node)
{
	load(node);
}

void AudioSubObjectSwitch::load(const ConfigNode& node)
{
	switchId = node["switchId"].asString();
	cases = node["cases"].asHashMap<String, AudioSubObjectHandle>();
}

ConfigNode AudioSubObjectSwitch::toConfigNode() const
{
	ConfigNode::MapType result;
	result["type"] = toString(getType());
	result["switchId"] = switchId;
	result["cases"] = cases;
	return result;
}

std::unique_ptr<AudioSource> AudioSubObjectSwitch::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	const auto& curValue = emitter.getSwitchValue(switchId);
	const auto iter = cases.find(curValue);
	if (iter != cases.end()) {
		return iter->second->makeSource(engine, emitter);
	}
	return {};
}

String AudioSubObjectSwitch::getName() const
{
	return "Switch [" + switchId + "]";
}

size_t AudioSubObjectSwitch::getNumSubObjects() const
{
	return cases.size();
}

AudioSubObjectHandle& AudioSubObjectSwitch::getSubObject(size_t n)
{
	// HACK
	size_t idx = 0;
	for (auto& c: cases) {
		if (idx == n) {
			return c.second;
		}
		++idx;
	}
	return IAudioSubObject::getSubObject(n);
}

Vector<String> AudioSubObjectSwitch::getSubCategories(const AudioProperties& audioProperties) const
{
	const auto* switchProperties = audioProperties.tryGetSwitch(switchId);

	Vector<String> result;
	if (switchProperties) {
		result.assign(switchProperties->getValues().begin(), switchProperties->getValues().end());
	}
	
	for (const auto& c: cases) {
		if (!std_ex::contains(result, c.first)) {
			result.push_back(c.first);
		}
	}
	return result;
}

String AudioSubObjectSwitch::getSubObjectCategory(size_t n) const
{
	// HACK
	size_t idx = 0;
	for (const auto& c: cases) {
		if (idx == n) {
			return c.first;
		}
		++idx;
	}
	return "";
}

bool AudioSubObjectSwitch::canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const
{
	if (!caseName.has_value()) {
		return false;
	}

	const auto iter = cases.find(caseName.value());
	if (iter == cases.end()) {
		return true;
	}

	return !iter->second.hasValue();
}

void AudioSubObjectSwitch::addObject(AudioSubObjectHandle audioSubObject, const std::optional<String>& caseName, size_t idx)
{
	cases[caseName.value()] = std::move(audioSubObject);
}

AudioSubObjectHandle AudioSubObjectSwitch::removeObject(const IAudioObject* object)
{
	for (auto& c: cases) {
		if (&c.second.getObject() == object) {
			return std::move(c.second);
		}
	}
	return AudioSubObjectHandle();
}

void AudioSubObjectSwitch::loadDependencies(Resources& resources)
{
	for (const auto& c: cases) {
		c.second->loadDependencies(resources);
	}
}

void AudioSubObjectSwitch::serialize(Serializer& s) const
{
	s << switchId;
	s << cases;
}

void AudioSubObjectSwitch::deserialize(Deserializer& s)
{
	s >> switchId;
	s >> cases;
}

const String& AudioSubObjectSwitch::getSwitchId() const
{
	return switchId;
}

void AudioSubObjectSwitch::setSwitchId(String value)
{
	switchId = std::move(value);
}
