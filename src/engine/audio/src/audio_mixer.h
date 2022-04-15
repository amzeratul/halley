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
	class AudioMixer
	{
	public:
		virtual ~AudioMixer() {}

		virtual void mixAudio(AudioSamplesConst src, AudioSamples dst, float gainStart, float gainEnd);
		void mixAudio(AudioMultiChannelSamplesConst src, AudioMultiChannelSamples dst, float gainStart, float gainEnd);
		void mixAudio(AudioMultiChannelSamples src, AudioMultiChannelSamples dst, float gainStart, float gainEnd);

		virtual void interleaveChannels(AudioSamples dst, gsl::span<AudioBuffer*> srcs);
		virtual void concatenateChannels(AudioSamples dst, gsl::span<AudioBuffer*> srcs);
		virtual void compressRange(AudioSamples buffer);

		void zero(AudioMultiChannelSamples dst, size_t nChannels = 8);
		void copy(AudioMultiChannelSamples src, AudioMultiChannelSamples dst, size_t nChannels = 8);
		void copy(AudioSamples src, AudioSamples dst);

		static std::unique_ptr<AudioMixer> makeMixer();
	};
}
