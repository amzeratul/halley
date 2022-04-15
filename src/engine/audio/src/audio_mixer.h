#pragma once
#include <gsl/span>
#include "halley/core/api/audio_api.h"
#include "audio_buffer.h"

namespace Halley
{
	class AudioMixer
	{
	public:
		static void mixAudio(AudioSamplesConst src, AudioSamples dst, float gainStart, float gainEnd);
		static void mixAudio(AudioMultiChannelSamplesConst src, AudioMultiChannelSamples dst, float gainStart, float gainEnd);
		static void mixAudio(AudioMultiChannelSamples src, AudioMultiChannelSamples dst, float gainStart, float gainEnd);

		static void interleaveChannels(AudioSamples dst, gsl::span<AudioBuffer*> srcs);
		static void concatenateChannels(AudioSamples dst, gsl::span<AudioBuffer*> srcs);
		static void compressRange(AudioSamples buffer);

		static void zero(AudioMultiChannelSamples dst, size_t nChannels = 8);
		static void copy(AudioMultiChannelSamples src, AudioMultiChannelSamples dst, size_t nChannels = 8);
		static void copy(AudioSamples src, AudioSamples dst);
	};
}
