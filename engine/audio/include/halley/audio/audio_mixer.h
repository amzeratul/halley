#pragma once
#include <gsl/span>
#include "halley/core/api/audio_api.h"
#include "audio_buffer.h"

#if defined(_M_X64) || defined(__x86_64__)
#define HAS_SSE
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

		virtual void mixAudio(gsl::span<const AudioSamplePack> src, gsl::span<AudioSamplePack> dst, float gainStart, float gainEnd);
		virtual void interleaveChannels(AudioBuffer& dstBuffer, gsl::span<const AudioBuffer> src);
		virtual void compressRange(gsl::span<AudioSamplePack> buffer);
		static std::unique_ptr<AudioMixer> makeMixer();
	};
}
