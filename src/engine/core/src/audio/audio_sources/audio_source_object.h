#pragma once
#include "halley/audio/audio_source.h"

namespace Halley
{
	class AudioEmitter;

	class AudioSourceObject final : public AudioSource
	{
	public:
		AudioSourceObject(AudioEngine& engine, AudioEmitter& emitter, const AudioObject& object);

		String getName() const override;

		bool isReady() const override;
		uint8_t getNumberOfChannels() const override;
		size_t getSamplesLeft() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		void restart() override;
		bool isLooping() override;

	private:
		const AudioObject& object;
		Vector<std::unique_ptr<AudioSource>> sources;
    };
}
