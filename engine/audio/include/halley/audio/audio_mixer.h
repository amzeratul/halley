#pragma once
#include <gsl/span>
#include "halley/core/api/audio_api.h"
#include "audio_buffer.h"

namespace Halley
{
	class AudioMixer
	{
	public:
		static void mixAudio(gsl::span<const AudioSamplePack> src, gsl::span<AudioSamplePack> dst, float gainStart, float gainEnd);
		static void interleaveChannels(AudioBuffer& dstBuffer, gsl::span<const AudioBuffer> src);
	};
}
