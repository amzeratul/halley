#include "halley/audio/sub_objects/audio_sub_object_layers.h"
#include "../audio_sources/audio_source_layers.h"
#include "halley/audio/audio_sub_object.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/utils/algorithm.h"

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
	name = node["name"].asString("");
}

ConfigNode AudioSubObjectLayers::toConfigNode() const
{
	ConfigNode::MapType result;

	result["type"] = toString(getType());
	if (!layers.empty()) {
		result["layers"] = layers;
	}
	if (fadeConfig.hasFade()) {
		result["fade"] = fadeConfig.toConfigNode();
	}
	result["name"] = name;
		
	return result;
}

String AudioSubObjectLayers::getName() const
{
	return name.isEmpty() ? "Layers" : name;
}

const String& AudioSubObjectLayers::getRawName() const
{
	return name;
}

void AudioSubObjectLayers::setName(String name)
{
	this->name = std::move(name);
}

size_t AudioSubObjectLayers::getNumSubObjects() const
{
	return layers.size();
}

AudioSubObjectHandle& AudioSubObjectLayers::getSubObject(size_t n)
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
	s << name;
	s << layers;
	s << fadeConfig;
}

void AudioSubObjectLayers::deserialize(Deserializer& s)
{
	s >> name;
	s >> layers;
	s >> fadeConfig;
}

const AudioSubObjectLayers::Layer& AudioSubObjectLayers::getLayer(size_t idx) const
{
	return layers.at(idx);
}

bool AudioSubObjectLayers::canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const
{
	return !caseName;
}

void AudioSubObjectLayers::addObject(AudioSubObjectHandle audioSubObject, const std::optional<String>& caseName, size_t idx)
{
	Layer layer;
	layer.object = std::move(audioSubObject);
	layers.insert(layers.begin() + std::min(layers.size(), idx), std::move(layer));
}

AudioSubObjectHandle AudioSubObjectLayers::removeObject(const IAudioObject* object)
{
	const auto iter = std_ex::find_if(layers, [&] (const Layer& layer) { return &layer.object.getObject() == object; });
	if (iter != layers.end()) {
		auto handle = std::move(iter->object);
		layers.erase(iter);
		return handle;
	}
	return AudioSubObjectHandle();
}

gsl::span<AudioSubObjectLayers::Layer> AudioSubObjectLayers::getLayers()
{
	return layers;
}

AudioFade& AudioSubObjectLayers::getFade()
{
	return fadeConfig;
}

AudioSubObjectLayers::Layer::Layer(const ConfigNode& node)
{
	object = IAudioSubObject::makeSubObject(node["object"]);
	expression.load(node["expression"]);
	synchronised = node["synchronised"].asBool(false);
	restartFromBeginning = node["restartFromBeginning"].asBool(false);
	onlyFadeInWhenResuming = node["onlyFadeInWhenResuming"].asBool(false);
	if (node.hasKey("fadeIn")) {
		fadeIn = AudioFade(node["fadeIn"]);
	}
	if (node.hasKey("fadeOut")) {
		fadeOut = AudioFade(node["fadeOut"]);
	}
	delay = node["delay"].asFloat(0);
}

ConfigNode AudioSubObjectLayers::Layer::toConfigNode() const
{
	ConfigNode::MapType result;
	result["object"] = object.toConfigNode();
	result["expression"] = expression.toConfigNode();
	if (synchronised) {
		result["synchronised"] = synchronised;
	}
	if (restartFromBeginning) {
		result["restartFromBeginning"] = restartFromBeginning;
	}
	if (onlyFadeInWhenResuming) {
		result["onlyFadeInWhenResuming"] = onlyFadeInWhenResuming;
	}
	if (fadeIn) {
		result["fadeIn"] = fadeIn->toConfigNode();
	}
	if (fadeOut) {
		result["fadeOut"] = fadeOut->toConfigNode();
	}
	if (delay > 0.0001f) {
		result["delay"] = delay;
	}
	return result;
}

void AudioSubObjectLayers::Layer::serialize(Serializer& s) const
{
	s << object;
	s << expression;
	s << synchronised;
	s << restartFromBeginning;
	s << onlyFadeInWhenResuming;
	s << fadeIn;
	s << fadeOut;
	s << delay;
}

void AudioSubObjectLayers::Layer::deserialize(Deserializer& s)
{
	s >> object;
	s >> expression;
	s >> synchronised;
	s >> restartFromBeginning;
	s >> onlyFadeInWhenResuming;
	s >> fadeIn;
	s >> fadeOut;
	s >> delay;
}
