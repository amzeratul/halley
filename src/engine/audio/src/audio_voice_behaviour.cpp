#include "audio_voice_behaviour.h"
#include "audio_voice.h"
#include "audio_facade.h"

using namespace Halley;

AudioVoiceBehaviour::~AudioVoiceBehaviour() {}
void AudioVoiceBehaviour::onAttach(AudioVoice& audioSource) {}

AudioVoiceFadeBehaviour::AudioVoiceFadeBehaviour(float fadeTime, float targetVolume, bool stopAtEnd)
	: curTime(0)
	, fadeTime(fadeTime)
	, volume0(0)
	, volume1(targetVolume)
	, stopAtEnd(stopAtEnd)
{	
}

void AudioVoiceFadeBehaviour::onAttach(AudioVoice& audioSource)
{
	volume0 = gainToVolume(audioSource.getGain());
}

bool AudioVoiceFadeBehaviour::update(float elapsedTime, AudioVoice& audioSource)
{
	curTime += elapsedTime;
	float t = clamp(curTime / fadeTime, 0.0f, 1.0f);
	float volume = lerp(volume0, volume1, t);
	
	audioSource.setGain(gainToVolume(volume));
	
	if (curTime >= fadeTime) {
		if (stopAtEnd) {
			audioSource.stop();
		}
		return false;
	}
	return true;
}
