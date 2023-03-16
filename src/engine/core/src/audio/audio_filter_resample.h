#pragma once
#include "halley/audio/audio_source.h"
#include "halley/audio/resampler.h"
#include "halley/audio/audio_buffer.h"

namespace Halley
{
	class AudioFilterResample final : public AudioSource
	{
	public:
		AudioFilterResample(std::shared_ptr<AudioSource> source, float fromHz, float toHz, AudioBufferPool& pool);

		uint8_t getNumberOfChannels() const override;
		bool isReady() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		size_t getSamplesLeft() const override;
		void restart() override;

		void setFromHz(float fromHz);

	private:
		AudioBufferPool& pool;
		std::shared_ptr<AudioSource> source;
		Vector<std::unique_ptr<AudioResampler>> resamplers;
		float fromHz;
		float toHz;

		struct LeftOverData
		{
			std::array<AudioSample, 8> samples;
			size_t n = 0;
		};
		std::array<LeftOverData, AudioConfig::maxChannels> leftoverSamples;
	};
}
