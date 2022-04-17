#include "audio_sub_object_switch.h"

#include "../audio_emitter.h"
#include "halley/bytes/byte_serializer.h"
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

std::unique_ptr<AudioSource> AudioSubObjectSwitch::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	const auto& curValue = emitter.getSwitchValue(switchId);
	const auto iter = cases.find(curValue);
	if (iter != cases.end()) {
		return iter->second->makeSource(engine, emitter);
	}
	return {};
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
