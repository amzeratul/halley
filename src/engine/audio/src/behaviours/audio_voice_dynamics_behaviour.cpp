#include "behaviours/audio_voice_dynamics_behaviour.h"

#include "audio_dynamics_config.h"
#include "../audio_engine.h"
#include "../audio_variable_table.h"

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
	const auto& vars = engine.getVariableTable();
	float& volume = audioSource.getDynamicGainRef();
	for (const auto& vol: config.getVolume()) {
		volume *= vol.getValue(vars.get(vol.name));
	}
	
	return true;
}
