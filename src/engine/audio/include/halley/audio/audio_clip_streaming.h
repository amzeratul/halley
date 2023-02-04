#pragma once
#include "audio_clip.h"
#include "halley/audio/resampler.h"
#include "halley/maths/rolling_data_set.h"

namespace Halley
{
	class AudioClipStreaming final : public IAudioClip
	{
	public:
		AudioClipStreaming(uint8_t numChannels);

		void addInterleavedSamples(AudioSamplesConst src);
		void addInterleavedSamplesWithResample(AudioSamplesConst src, float sourceSampleRate);
		void addInterleavedSamplesWithResampleSync(AudioSamplesConst src, float sourceSampleRate, float maxPitchShift);

		size_t copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const override;
		uint8_t getNumberOfChannels() const override;
		size_t getLength() const override;
		size_t getSamplesLeft() const;
		bool isLoaded() const override;

	private:
		std::atomic_size_t length;
		mutable std::atomic_size_t samplesLeft;
		mutable Vector<Vector<AudioSample>> buffers;
		mutable std::mutex mutex;
		uint8_t numChannels = 0;
		bool ready = false;

		std::unique_ptr<AudioResampler> resampler;
		Vector<float> pending;
		Vector<float> resampleDstBuffer;
		Vector<float> resampleSrcBuffer;

		RollingDataSet<size_t> samplesLeftAvg;

		void doAddInterleavedSamplesWithResample(AudioSamplesConst src);
		void updateSync(float maxPitchShift, float sourceSampleRate);
	};
}
