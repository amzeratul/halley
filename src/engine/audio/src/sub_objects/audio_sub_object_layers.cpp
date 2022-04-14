#include "audio_sub_object_layers.h"
using namespace Halley;

void AudioSubObjectLayers::load(const ConfigNode& node)
{
}

std::unique_ptr<AudioSource> AudioSubObjectLayers::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	return {};
}

void AudioSubObjectLayers::loadDependencies(Resources& resources)
{
}

void AudioSubObjectLayers::serialize(Serializer& s) const
{
}

void AudioSubObjectLayers::deserialize(Deserializer& s)
{
}
