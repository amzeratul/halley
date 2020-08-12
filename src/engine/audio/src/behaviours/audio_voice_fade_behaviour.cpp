#include "behaviours/audio_voice_fade_behaviour.h"
#include "../audio_voice.h"
#include "audio_facade.h"
#include "halley/utils/utils.h"

using namespace Halley;

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
