#include "audio_source_behaviour.h"
#include "audio_source.h"

using namespace Halley;

AudioSourceBehaviour::~AudioSourceBehaviour() {}
void AudioSourceBehaviour::onAttach(AudioSource& audioSource) {}

AudioSourceFadeBehaviour::AudioSourceFadeBehaviour(float fadeTime, float targetGain, bool stopAtEnd)
	: curTime(0)
	, fadeTime(fadeTime)
	, gain0(0)
	, gain1(targetGain)
	, stopAtEnd(stopAtEnd)
{	
}

void AudioSourceFadeBehaviour::onAttach(AudioSource& audioSource)
{
	gain0 = audioSource.getGain();
}

bool AudioSourceFadeBehaviour::update(float elapsedTime, AudioSource& audioSource)
{
	curTime += elapsedTime;
	float t = clamp(curTime / fadeTime, 0.0f, 1.0f);
	float gain = lerp(gain0, gain1, t);
	audioSource.setGain(gain);
	if (curTime >= fadeTime) {
		if (stopAtEnd) {
			audioSource.stop();
		}
		return false;
	}
	return true;
}
