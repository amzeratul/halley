#include "halley/audio/sub_objects/audio_sub_object_layers.h"
#include "../audio_sources/audio_source_layers.h"
#include "halley/audio/audio_sub_object.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/maths/uuid.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

void AudioSubObjectLayers::load(const ConfigNode& node)
{
	setId(node["id"].asString(""));
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

	result["id"] = getId();
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
	return objectName + ":" + name;
}

const String& AudioSubObjectLayers::getRawName() const
{
	return name;
}

void AudioSubObjectLayers::setName(String name)
{
	this->name = std::move(name);
}

void AudioSubObjectLayers::setObjectName(const String& name)
{
	objectName = name;
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
	AudioSubObject::serialize(s);
	s << name;
	s << layers;
	s << fadeConfig;
}

void AudioSubObjectLayers::deserialize(Deserializer& s)
{
	AudioSubObject::deserialize(s);
	s >> name;
	s >> layers;
	s >> fadeConfig;
}

bool AudioSubObjectLayers::reload(AudioSubObject&& otherRaw)
{
	auto&& other = dynamic_cast<AudioSubObjectLayers&&>(std::move(otherRaw));

	bool modified = false;
	if (name != other.name) {
		name = std::move(other.name);
		modified = true;
	}
	if (objectName != other.objectName) {
		objectName = std::move(other.objectName);
		modified = true;
	}
	if (fadeConfig != other.fadeConfig) {
		fadeConfig = other.fadeConfig;
		modified = true;
	}

	auto oldLayers = std::move(layers);
	layers = {};
	for (auto& newLayer: other.layers) {
		auto iter = oldLayers.find_if([&] (const Layer& layer) {
			return layer.getId() == newLayer.getId();
		});

		if (iter != oldLayers.end()) {
			auto oldLayer = std::move(*iter);
			modified = oldLayer.reload(std::move(newLayer)) || modified;
			layers.push_back(std::move(oldLayer));
		} else {
			modified = true;
			layers.push_back(std::move(newLayer));
		}
	}

	for (const auto& oldLayer: oldLayers) {
		if (oldLayer.object.hasValue()) {
			// Removed
			modified = true;
		}
	}

	return modified;
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

void AudioSubObjectLayers::validate(const AudioProperties& audioProperties) const
{
	for (auto& layer: layers) {
		layer.expression.validate(audioProperties, getName());
	}
}

AudioSubObjectLayers::Layer::Layer(const ConfigNode& node)
{
	object = AudioSubObject::makeSubObject(node["object"]);
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

const String& AudioSubObjectLayers::Layer::getId() const
{
	return object.getId();
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

bool AudioSubObjectLayers::Layer::reload(Layer&& other)
{
	bool modified = false;

	if (expression != other.expression) {
		expression = other.expression;
		modified = true;
	}
	if (fadeIn != other.fadeIn) {
		fadeIn = other.fadeIn;
		modified = true;
	}
	if (fadeOut != other.fadeOut) {
		fadeOut = other.fadeOut;
		modified = true;
	}
	if (delay != other.delay) {
		delay = other.delay;
		modified = true;
	}
	if (synchronised != other.synchronised) {
		synchronised = other.synchronised;
		modified = true;
	}
	if (restartFromBeginning != other.restartFromBeginning) {
		restartFromBeginning = other.restartFromBeginning;
		modified = true;
	}
	if (onlyFadeInWhenResuming != other.onlyFadeInWhenResuming) {
		onlyFadeInWhenResuming = other.onlyFadeInWhenResuming;
		modified = true;
	}

	modified = object.reload(std::move(other.object)) || modified;

	return modified;
}
