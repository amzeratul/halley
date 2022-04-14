#pragma once
#include <gsl/span>
#include "halley/core/api/audio_api.h"
#include "audio_buffer.h"

#if defined(_M_X64) || defined(__x86_64__)
#define HAS_SSE
#if !defined(__linux__)
#define HAS_AVX
#endif
#endif

#if defined(_M_IX86) || defined(__i386)
// Might not be available, but do we really care about such old processors?
#define HAS_SSE
#endif

namespace Halley
{
	using AudioChannelSamples = gsl::span<AudioSamplePack>;
	using AudioChannelSamplesConst = gsl::span<const AudioSamplePack>;
	using AudioMultiChannelSamples = std::array<gsl::span<AudioSamplePack>, AudioConfig::maxChannels>;
	using AudioMultiChannelSamplesConst = std::array<gsl::span<const AudioSamplePack>, AudioConfig::maxChannels>;

	class AudioMixer
	{
	public:
		virtual ~AudioMixer() {}

		virtual void mixAudio(AudioChannelSamplesConst src, AudioChannelSamples dst, float gainStart, float gainEnd);
		void mixAudio(AudioMultiChannelSamplesConst src, AudioMultiChannelSamples dst, float gainStart, float gainEnd);
		void mixAudio(AudioMultiChannelSamples src, AudioMultiChannelSamples dst, float gainStart, float gainEnd);

		virtual void interleaveChannels(gsl::span<AudioSamplePack> dst, gsl::span<AudioBuffer*> srcs);
		virtual void concatenateChannels(gsl::span<AudioSamplePack> dst, gsl::span<AudioBuffer*> srcs);
		virtual void compressRange(gsl::span<AudioSamplePack> buffer);

		void zero(AudioMultiChannelSamples dst, size_t nChannels = 8);
		void copy(AudioMultiChannelSamples src, std::array<gsl::span<AudioConfig::SampleFormat>, AudioConfig::maxChannels> dst, size_t nChannels = 8);
		void copy(AudioChannelSamples src, gsl::span<AudioConfig::SampleFormat> dst);

		static std::unique_ptr<AudioMixer> makeMixer();
	};
}
