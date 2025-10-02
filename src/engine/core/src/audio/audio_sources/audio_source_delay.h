#pragma once
#include "halley/audio/audio_source.h"

namespace Halley
{
	class AudioSourceDelay final : public AudioSource
	{
	public:
		AudioSourceDelay(std::shared_ptr<AudioSource> src, size_t delay);

		void setSource(std::shared_ptr<AudioSource> source);

		String getName() const override;
		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		bool isReady() const override;
		size_t getSamplesLeft() const override;
        void restart() override;
		void setInitialDelay(size_t delay);
		bool isLooping() override;

	private:
		std::shared_ptr<AudioSource> src;
        size_t initialDelay;
		size_t curDelay;
	};
}
