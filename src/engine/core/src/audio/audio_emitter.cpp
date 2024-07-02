#include "audio_emitter.h"

#include "halley/utils/algorithm.h"

using namespace Halley;

AudioEmitter::AudioEmitter(AudioEmitterId id, AudioPosition position, bool temporary, AudioEmitter* fallback)
	: id(id)
	, temporary(temporary)
	, position(std::move(position))
	, fallback(fallback)
{
}

AudioEmitterId AudioEmitter::getId() const
{
	return id;
}

const AudioPosition& AudioEmitter::getPosition() const
{
	return position;
}

void AudioEmitter::setPosition(const AudioPosition& pos)
{
	position = pos;
}

void AudioEmitter::addVoice(std::unique_ptr<AudioVoice> voice)
{
	voices.push_back(std::move(voice));
}

void AudioEmitter::removeFinishedVoices(Vector<AudioEventId>& removedIds, Vector<AudioObjectId>& removedObjects)
{
	std_ex::erase_if(voices, [&] (const auto& v)
	{
		const bool done = v->isDone();
		if (done) {
			removedIds.push_back(v->getEventId());
			removedObjects.push_back(v->getAudioObjectId());
		}
		return done;
	});
}

gsl::span<const std::unique_ptr<AudioVoice>> AudioEmitter::getVoices() const
{
	return voices;
}

size_t AudioEmitter::forVoices(AudioObjectId audioObjectId, VoiceCallback callback)
{
	size_t n = 0;
	for (auto& v: voices) {
		if (v->getAudioObjectId() == audioObjectId) {
			callback(*v);
			++n;
		}
	}
	return n;
}

bool AudioEmitter::shouldBeRemoved()
{
	return temporary && voices.empty();
}

void AudioEmitter::makeTemporary()
{
	temporary = true;
}

void AudioEmitter::setSwitchValue(const String& id, String value)
{
	switchValues[id] = std::move(value);
}

void AudioEmitter::setVariableValue(const String& id, float value)
{
	variableValues[id] = value;
}

const String& AudioEmitter::getSwitchValue(const String& id) const
{
	const auto iter = switchValues.find(id);
	if (iter == switchValues.end()) {
		return fallback ? fallback->getSwitchValue(id) : String::emptyString();
	}
	return iter->second;
}

float AudioEmitter::getVariableValue(const String& id) const
{
	const auto iter = variableValues.find(id);
	if (iter == variableValues.end()) {
		return fallback ? fallback->getVariableValue(id) : 0;
	}
	return iter->second;
}

void AudioEmitter::setRegion(AudioRegionId regionId)
{
	this->regionId = regionId;
}

AudioRegionId AudioEmitter::getRegion() const
{
	return regionId;
}

AudioDebugData::EmitterData AudioEmitter::getDebugData() const
{
	AudioDebugData::EmitterData result;

	result.emitterId = id;
	result.switches = switchValues;
	result.variables = variableValues;
	result.regionId = regionId;

	result.voices.reserve(voices.size());
	for (const auto& voice: voices) {
		result.voices.emplace_back(voice->getDebugData());
	}

	return result;
}
