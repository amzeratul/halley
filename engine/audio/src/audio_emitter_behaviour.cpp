#include "audio_emitter_behaviour.h"
#include "audio_emitter.h"

using namespace Halley;

AudioEmitterBehaviour::~AudioEmitterBehaviour() {}
void AudioEmitterBehaviour::onAttach(AudioEmitter& audioSource) {}

AudioEmitterFadeBehaviour::AudioEmitterFadeBehaviour(float fadeTime, float targetGain, bool stopAtEnd)
	: curTime(0)
	, fadeTime(fadeTime)
	, gain0(0)
	, gain1(targetGain)
	, stopAtEnd(stopAtEnd)
{	
}

void AudioEmitterFadeBehaviour::onAttach(AudioEmitter& audioSource)
{
	gain0 = audioSource.getGain();
}

bool AudioEmitterFadeBehaviour::update(float elapsedTime, AudioEmitter& audioSource)
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
