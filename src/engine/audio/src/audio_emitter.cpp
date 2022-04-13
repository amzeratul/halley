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

void AudioEmitter::removeFinishedVoices()
{
	std_ex::erase_if(voices, [&] (const auto& v) { return v->isDone(); });
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
