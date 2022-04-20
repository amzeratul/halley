#include "audio_sub_object_switch.h"

#include "../audio_emitter.h"
#include "halley/bytes/byte_serializer.h"
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

Vector<String> AudioSubObjectSwitch::getSubCategories() const
{
	// TODO: read from switch
	Vector<String> result;
	for (auto& c: cases) {
		result.push_back(c.first);
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

bool AudioSubObjectSwitch::canAddObject(const std::optional<String>& caseName) const
{
	return caseName.has_value() && cases.find(caseName.value()) == cases.end();
}

void AudioSubObjectSwitch::addObject(AudioSubObjectHandle audioSubObject, const std::optional<String>& caseName, size_t idx)
{
	cases[caseName.value()] = std::move(audioSubObject);
}

void AudioSubObjectSwitch::removeObject(const IAudioObject* object)
{
	for (auto& c: cases) {
		if (&c.second.getObject() == object) {
			c.second = AudioSubObjectHandle();
			return;
		}
	}
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
