#include "sub_objects/audio_sub_object_sequence.h"

#include "halley/utils/algorithm.h"
#include "halley/bytes/byte_serializer.h"
#include "../audio_sources/audio_source_sequence.h"


using namespace Halley;

void AudioSubObjectSequence::load(const ConfigNode& node)
{
	segments = node["segments"].asVector<AudioSubObjectHandle>({});
	crossFade = AudioFade(node["crossFade"]);
	sequenceType = fromString<AudioSequenceType>(node["sequenceType"].asString("sequential"));
}

ConfigNode AudioSubObjectSequence::toConfigNode() const
{
	ConfigNode::MapType result;
	result["type"] = toString(getType());
	result["segments"] = segments;
	result["crossFade"] = crossFade.toConfigNode();
	result["sequenceType"] = toString(sequenceType);
	return result;
}

std::unique_ptr<AudioSource> AudioSubObjectSequence::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	return std::make_unique<AudioSourceSequence>(engine, emitter, *this);
}

String AudioSubObjectSequence::getName() const
{
	return "Sequence";
}

size_t AudioSubObjectSequence::getNumSubObjects() const
{
	return segments.size();
}

const AudioSubObjectHandle& AudioSubObjectSequence::getSubObject(size_t n) const
{
	return segments.at(n);
}

AudioSubObjectHandle& AudioSubObjectSequence::getSubObject(size_t n)
{
	return segments.at(n);
}

bool AudioSubObjectSequence::canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const
{
	return true;
}

void AudioSubObjectSequence::addObject(AudioSubObjectHandle handle, const std::optional<String>& caseName, size_t idx)
{
	segments.insert(segments.begin() + std::min(segments.size(), idx), std::move(handle));
}

AudioSubObjectHandle AudioSubObjectSequence::removeObject(const IAudioObject* object)
{
	auto iter = std_ex::find_if(segments, [&] (const auto& e) { return &e.getObject() == object; });
	if (iter != segments.end()) {
		auto handle = std::move(*iter);
		segments.erase(iter);
		return handle;
	}
	return AudioSubObjectHandle();
}

void AudioSubObjectSequence::loadDependencies(Resources& resources)
{
	for (auto& s: segments) {
		s->loadDependencies(resources);
	}
}

void AudioSubObjectSequence::serialize(Serializer& s) const
{
	s << segments;
	s << sequenceType;
}

void AudioSubObjectSequence::deserialize(Deserializer& s)
{
	s >> segments;
	s >> sequenceType;
}

AudioFade& AudioSubObjectSequence::getCrossFade()
{
	return crossFade;
}

AudioSequenceType& AudioSubObjectSequence::getSequenceType()
{
	return sequenceType;
}

const AudioFade& AudioSubObjectSequence::getCrossFade() const
{
	return crossFade;
}

AudioSequenceType AudioSubObjectSequence::getSequenceType() const
{
	return sequenceType;
}
