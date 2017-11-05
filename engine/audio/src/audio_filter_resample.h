#pragma once
#include "audio_source.h"
#include "halley/audio/resampler.h"
#include "audio_buffer.h"

namespace Halley
{
	class AudioFilterResample : public AudioSource
	{
	public:
		AudioFilterResample(std::shared_ptr<AudioSource> source, int fromHz, int toHz, AudioBufferPool& pool);

		size_t getNumberOfChannels() const override;
		bool isReady() const override;
		bool getAudioData(size_t numSamples, AudioSourceData dst) override;

	private:
		AudioBufferPool& pool;
		std::shared_ptr<AudioSource> source;
		std::vector<std::unique_ptr<AudioResampler>> resamplers;
		int fromHz;
		int toHz;

		struct LeftOverData
		{
			std::array<AudioConfig::SampleFormat, 8> samples;
			size_t n = 0;
		};
		std::array<LeftOverData, AudioConfig::maxChannels> leftoverSamples;
	};
}
