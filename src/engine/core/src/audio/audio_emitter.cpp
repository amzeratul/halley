#include "audio_emitter.h"

#include "audio_engine.h"
#include "halley/api/audio_api.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

AudioEmitter::AudioEmitter(AudioEngine& engine, AudioEmitterId id, AudioPosition position, bool temporary, AudioEmitter* fallback)
	: engine(engine)
	, id(id)
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

void AudioEmitter::removeFinishedVoices(Vector<AudioEventId>& removedIds)
{
	std_ex::erase_if(voices, [&] (const auto& v)
	{
		const bool done = v->isDone();
		if (done) {
			removedIds.push_back(v->getEventId());
			engine.onVoiceFinished(*v);
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
		if (fallback) {
			return fallback->getSwitchValue(id);
		} else {
			const auto& value = engine.getSwitchDefault(id);
			switchValues[id] = value;
			return value;
		}
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

namespace {
	
	float getTotalMixAmount(const AudioDebugData::VoiceData& voiceData)
	{
		if (voiceData.dstChannels == 0) {
			return 0.0f;
		}
		float result = 0;
		for (size_t i = 0; i < static_cast<size_t>(voiceData.srcChannels) * static_cast<size_t>(voiceData.dstChannels); ++i) {
			result += voiceData.channelMix[i];
		}
		return result / static_cast<float>(voiceData.dstChannels);
	}
}

AudioDebugData::EmitterData AudioEmitter::getDebugData() const
{
	AudioDebugData::EmitterData result;

	result.emitterId = id;
	result.switches = switchValues;
	result.variables = variableValues;
	result.regionId = regionId;
	result.totalMix = 0;

	result.voices.reserve(voices.size());
	for (const auto& voice: voices) {
		result.voices.emplace_back(voice->getDebugData());
		result.totalMix += getTotalMixAmount(result.voices.back());
	}

	return result;
}
