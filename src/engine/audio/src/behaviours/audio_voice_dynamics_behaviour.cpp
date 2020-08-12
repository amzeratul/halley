#include "behaviours/audio_voice_dynamics_behaviour.h"

using namespace Halley;

AudioVoiceDynamicsBehaviour::AudioVoiceDynamicsBehaviour(const AudioDynamicsConfig& config, AudioEngine& engine)
	: config(config)
	, engine(engine)
{
}

void AudioVoiceDynamicsBehaviour::onAttach(AudioVoice& audioSource)
{
}

bool AudioVoiceDynamicsBehaviour::update(float elapsedTime, AudioVoice& audioSource)
{
	// TODO
	return true;
}
