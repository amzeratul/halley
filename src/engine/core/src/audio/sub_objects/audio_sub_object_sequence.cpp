#include "halley/audio/sub_objects/audio_sub_object_sequence.h"

#include "halley/utils/algorithm.h"
#include "halley/bytes/byte_serializer.h"
#include "../audio_sources/audio_source_sequence.h"


using namespace Halley;

void AudioSubObjectSequence::load(const ConfigNode& node)
{
	name = node["name"].asString("");
	segments = node["segments"].asVector<Segment>({});
	crossFade = AudioFade(node["crossFade"]);
	sequenceType = fromString<AudioSequenceType>(node["sequenceType"].asString("sequential"));
}

ConfigNode AudioSubObjectSequence::toConfigNode() const
{
	ConfigNode::MapType result;
	result["name"] = name;
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
	return name.isEmpty() ? "Sequence" : name;
}

const String& AudioSubObjectSequence::getRawName() const
{
	return name;
}

void AudioSubObjectSequence::setName(String name)
{
	this->name = std::move(name);
}

size_t AudioSubObjectSequence::getNumSubObjects() const
{
	return segments.size();
}

const AudioSubObjectHandle& AudioSubObjectSequence::getSubObject(size_t n) const
{
	return segments.at(n).object;
}

AudioSubObjectHandle& AudioSubObjectSequence::getSubObject(size_t n)
{
	return segments.at(n).object;
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
	auto iter = std_ex::find_if(segments, [&] (const Segment& s) { return &s.object.getObject() == object; });
	if (iter != segments.end()) {
		auto handle = std::move(*iter);
		segments.erase(iter);
		return handle.object;
	}
	return AudioSubObjectHandle();
}

void AudioSubObjectSequence::loadDependencies(Resources& resources)
{
	for (auto& s: segments) {
		s.object->loadDependencies(resources);
	}
}

void AudioSubObjectSequence::serialize(Serializer& s) const
{
	s << name;
	s << segments;
	s << crossFade;
	s << sequenceType;
}

void AudioSubObjectSequence::deserialize(Deserializer& s)
{
	s >> name;
	s >> segments;
	s >> crossFade;
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

gsl::span<const AudioSubObjectSequence::Segment> AudioSubObjectSequence::getSegments() const
{
	return segments;
}

gsl::span<AudioSubObjectSequence::Segment> AudioSubObjectSequence::getSegments()
{
	return segments;
}

AudioSubObjectSequence::Segment& AudioSubObjectSequence::getSegment(size_t idx)
{
	return segments.at(idx);
}


AudioSubObjectSequence::Segment::Segment(AudioSubObjectHandle subObject)
	: object(std::move(subObject))
{
}

AudioSubObjectSequence::Segment::Segment(const ConfigNode& node)
{
	object = node["object"];
	endSample = node["endSample"].asInt(0);
}

ConfigNode AudioSubObjectSequence::Segment::toConfigNode() const
{
	ConfigNode::MapType result;
	result["object"] = object.toConfigNode();
	result["endSample"] = endSample;
	return result;
}

void AudioSubObjectSequence::Segment::serialize(Serializer& s) const
{
	s << object;
	s << endSample;
}

void AudioSubObjectSequence::Segment::deserialize(Deserializer& s)
{
	s >> object;
	s >> endSample;
}
