#include "audio_sub_object_layers.h"
#include "halley/bytes/byte_serializer.h"
#include "../audio_sources/audio_source_layers.h"

using namespace Halley;

void AudioSubObjectLayers::load(const ConfigNode& node)
{
	if (node.hasKey("layers")) {
		for (const auto& layerNode: node["layers"]) {
			layers.emplace_back(layerNode);
		}
	}
}

std::unique_ptr<AudioSource> AudioSubObjectLayers::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	Vector<std::unique_ptr<AudioSource>> sources;
	sources.reserve(layers.size());
	for (auto& l: layers) {
		sources.push_back(l.object->makeSource(engine, emitter));
	}

	return std::make_unique<AudioSourceLayers>(engine, emitter, std::move(sources), *this);
}

void AudioSubObjectLayers::loadDependencies(Resources& resources)
{
	for (auto& l: layers) {
		l.object->loadDependencies(resources);
	}
}

void AudioSubObjectLayers::serialize(Serializer& s) const
{
	s << layers;
}

void AudioSubObjectLayers::deserialize(Deserializer& s)
{
	s >> layers;
}

const AudioExpression& AudioSubObjectLayers::getLayerExpression(size_t idx) const
{
	return layers.at(idx).expression;
}

bool AudioSubObjectLayers::isLayerSynchronised(size_t idx) const
{
	return layers.at(idx).synchronised;
}

AudioSubObjectLayers::Layer::Layer(const ConfigNode& node)
{
	object = IAudioSubObject::makeSubObject(node["object"]);
	expression.load(node["expression"]);
	synchronised = node["synchronised"].asBool(false);
}

void AudioSubObjectLayers::Layer::serialize(Serializer& s) const
{
	s << object;
	s << expression;
	s << synchronised;
}

void AudioSubObjectLayers::Layer::deserialize(Deserializer& s)
{
	s >> object;
	s >> expression;
	s >> synchronised;
}
