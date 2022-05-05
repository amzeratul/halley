#include "audio_source_sequence.h"
using namespace Halley;

AudioSourceSequence::AudioSourceSequence(AudioEngine& engine, AudioEmitter& emitter, const AudioSubObjectSequence& sequenceConfig, AudioFade fadeConfig)
	: engine(engine)
	, emitter(emitter)
	, sequenceConfig(sequenceConfig)
	, fadeConfig(fadeConfig)
{
}

uint8_t AudioSourceSequence::getNumberOfChannels() const
{
	// TODO
	return 0;
}

bool AudioSourceSequence::getAudioData(size_t numSamples, AudioMultiChannelSamples dst)
{
	// TODO
	return false;
}

bool AudioSourceSequence::isReady() const
{
	// TODO
	return false;
}
