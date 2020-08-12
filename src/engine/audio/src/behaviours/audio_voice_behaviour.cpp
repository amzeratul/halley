#include "behaviours/audio_voice_behaviour.h"

using namespace Halley;

AudioVoiceBehaviour::~AudioVoiceBehaviour() {}

void AudioVoiceBehaviour::onAttach(AudioVoice& audioSource) {}

bool AudioVoiceBehaviour::updateChain(float elapsedTime, AudioVoice& audioSource)
{
	if (next) {
		const bool keepNext = next->updateChain(elapsedTime, audioSource);
		if (!keepNext) {
			next = next->releaseNext();
		}
	}
	return update(elapsedTime, audioSource);
}

void AudioVoiceBehaviour::addToChain(std::unique_ptr<AudioVoiceBehaviour> n)
{
	if (next) {
		next->addToChain(std::move(n));
	} else {
		next = std::move(n);
	}
}

std::unique_ptr<AudioVoiceBehaviour> AudioVoiceBehaviour::releaseNext()
{
	return std::move(next);
}
