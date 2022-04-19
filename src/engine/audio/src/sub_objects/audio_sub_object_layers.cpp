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
	if (node.hasKey("fade")) {
		fadeConfig = AudioFade(node["fade"]);
	} else {
		fadeConfig = AudioFade(1.0f, AudioFadeCurve::Linear);
	}
}

ConfigNode AudioSubObjectLayers::toConfigNode() const
{
	ConfigNode::MapType result;

	if (!layers.empty()) {
		result["layers"] = layers;
	}
	if (fadeConfig.hasFade()) {
		result["fade"] = fadeConfig.toConfigNode();
	}
		
	return result;
}

String AudioSubObjectLayers::getName() const
{
	return "Layers";
}

size_t AudioSubObjectLayers::getNumSubObjects() const
{
	return layers.size();
}

const AudioSubObjectHandle& AudioSubObjectLayers::getSubObject(size_t n) const
{
	return layers[n].object;
}

std::unique_ptr<AudioSource> AudioSubObjectLayers::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	Vector<std::unique_ptr<AudioSource>> sources;
	sources.reserve(layers.size());
	for (auto& l: layers) {
		sources.push_back(l.object->makeSource(engine, emitter));
	}

	return std::make_unique<AudioSourceLayers>(engine, emitter, std::move(sources), *this, fadeConfig);
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
	s << fadeConfig;
}

void AudioSubObjectLayers::deserialize(Deserializer& s)
{
	s >> layers;
	s >> fadeConfig;
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

ConfigNode AudioSubObjectLayers::Layer::toConfigNode() const
{
	ConfigNode::MapType result;
	result["object"] = object.toConfigNode();
	result["expression"] = expression.toConfigNode();
	result["synchronised"] = synchronised;
	return result;
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
