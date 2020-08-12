#include "audio_voice_behaviour.h"
#include "audio_voice.h"
#include "audio_facade.h"

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

AudioVoiceFadeBehaviour::AudioVoiceFadeBehaviour(float fadeTime, float sourceVolume, float targetVolume, bool stopAtEnd)
	: curTime(0)
	, fadeTime(fadeTime)
	, volume0(sourceVolume)
	, volume1(targetVolume)
	, stopAtEnd(stopAtEnd)
{	
}

void AudioVoiceFadeBehaviour::onAttach(AudioVoice& audioSource)
{
}

bool AudioVoiceFadeBehaviour::update(float elapsedTime, AudioVoice& audioSource)
{
	curTime += elapsedTime;
	const float t = clamp(curTime / fadeTime, 0.0f, 1.0f);
	const float volume = lerp(volume0, volume1, t);
	
	audioSource.getDynamicGainRef() *= gainToVolume(volume);
	
	if (curTime >= fadeTime) {
		if (stopAtEnd) {
			audioSource.stop();
		}
		return false;
	}
	return true;
}
