#include "audio_emitter.h"

#include "halley/utils/algorithm.h"

using namespace Halley;

AudioEmitter::AudioEmitter(AudioEmitterId id, AudioPosition position, bool temporary)
	: id(id)
	, temporary(temporary)
	, position(std::move(position))
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
		}
		return done;
	});
}

gsl::span<const std::unique_ptr<AudioVoice>> AudioEmitter::getVoices() const
{
	return voices;
}

void AudioEmitter::forVoices(AudioObjectId audioObjectId, VoiceCallback callback)
{
	for (auto& v: voices) {
		if (v->getAudioObjectId() == audioObjectId) {
			callback(*v);
		}
	}
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
		static String emptyString;
		return emptyString;
	}
	return iter->second;
}

float AudioEmitter::getVariableValue(const String& id) const
{
	const auto iter = variableValues.find(id);
	if (iter == variableValues.end()) {
		return 0;
	}
	return iter->second;
}
