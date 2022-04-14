#include "audio_sub_object_layers.h"
#include "halley/bytes/byte_serializer.h"
#include "../audio_sources/audio_source_layers.h"

using namespace Halley;

void AudioSubObjectLayers::load(const ConfigNode& node)
{
}

std::unique_ptr<AudioSource> AudioSubObjectLayers::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	Vector<std::unique_ptr<AudioSource>> sources;
	sources.reserve(layerObjects.size());
	for (auto& l: layerObjects) {
		sources.push_back(l->makeSource(engine, emitter));
	}

	return std::make_unique<AudioSourceLayers>(engine, emitter, std::move(sources));
}

void AudioSubObjectLayers::loadDependencies(Resources& resources)
{
	for (auto& l: layerObjects) {
		l->loadDependencies(resources);
	}
}

void AudioSubObjectLayers::serialize(Serializer& s) const
{
	s << layerObjects;
}

void AudioSubObjectLayers::deserialize(Deserializer& s)
{
	s >> layerObjects;
}
