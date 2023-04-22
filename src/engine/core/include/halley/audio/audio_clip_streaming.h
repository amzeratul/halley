#pragma once
#include "audio_clip.h"
#include "halley/api/core_api.h"
#include "halley/audio/resampler.h"
#include "halley/maths/rolling_data_set.h"

namespace Halley
{
	class AudioClipStreaming final : public IAudioClip, CoreAPI::IStartFrameCallback, public IAudioBufferSizeController
	{
	public:
		AudioClipStreaming(uint8_t numChannels);
		~AudioClipStreaming() override;

		void addInterleavedSamples(AudioSamplesConst src);
		void addInterleavedSamplesWithResample(AudioSamplesConst src, float sourceSampleRate);
		void addInterleavedSamplesWithResampleSync(AudioSamplesConst src, float sourceSampleRate, float maxPitchShift, CoreAPI& core, AudioOutputAPI& audioOut);

		size_t copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const override;
		uint8_t getNumberOfChannels() const override;
		size_t getLength() const override;
		size_t getSamplesLeft() const;
		bool isLoaded() const override;

		void setLatencyTarget(size_t samples);
		size_t getLatencyTarget() const;

		void setPaused(bool paused);

		size_t getTargetSamples() const override;

	private:
		std::atomic_size_t length;
		mutable std::atomic_size_t samplesLeft;
		mutable Vector<RingBuffer<float>> buffers;
		mutable std::mutex mutex;

		uint8_t numChannels = 0;
		bool ready = false;
		bool paused = false;
		size_t latencyTarget = 512;

		std::unique_ptr<AudioResampler> resampler;
		Vector<float> pending;
		Vector<float> resampleDstBuffer;
		Vector<float> resampleSrcBuffer;

		RollingDataSet<size_t> samplesLeftAvg;
		CoreAPI* coreAPI = nullptr;
		AudioOutputAPI* audioOutAPI = nullptr;
		float sourceSampleRate = 48000.0f;
		float maxPitchShift = 0.001f;

		void doAddInterleavedSamplesWithResample(AudioSamplesConst src);
		void onStartFrame() override;
	};
}
