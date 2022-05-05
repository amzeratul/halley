#pragma once
#include "audio_fade.h"
#include "audio_source.h"
#include "sub_objects/audio_sub_object_sequence.h"

namespace Halley
{
	class AudioSourceSequence final : public AudioSource
	{
	public:
		AudioSourceSequence(AudioEngine& engine, AudioEmitter& emitter, const AudioSubObjectSequence& sequenceConfig, AudioFade fadeConfig);

		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		bool isReady() const override;

	private:
		AudioEngine& engine;
		AudioEmitter& emitter;
		const AudioSubObjectSequence& sequenceConfig;
		AudioFade fadeConfig;
	};
}
