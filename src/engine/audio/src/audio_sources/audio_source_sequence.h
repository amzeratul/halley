#pragma once
#include "audio_fade.h"
#include "audio_source.h"
#include "sub_objects/audio_sub_object_sequence.h"

namespace Halley
{
	class AudioSourceSequence final : public AudioSource
	{
	public:
		AudioSourceSequence(AudioEngine& engine, AudioEmitter& emitter, const AudioSubObjectSequence& sequenceConfig);

		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		bool isReady() const override;
		size_t getSamplesLeft() const override;

	private:
		AudioEngine& engine;
		AudioEmitter& emitter;
		const AudioSubObjectSequence& sequenceConfig;

		Vector<size_t> playList;
		size_t curTrack = 0;

		bool initialized = false;

		void initialize();
		void nextTrack();
	};
}
