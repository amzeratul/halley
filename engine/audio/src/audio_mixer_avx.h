#pragma once
#include "audio_mixer.h"

#ifdef HAS_SSE
namespace Halley
{
	class AudioMixerAVX : public AudioMixer
	{
	public:
		void mixAudio(gsl::span<const AudioSamplePack> src, gsl::span<AudioSamplePack> dst, float gainStart, float gainEnd) override;
		void compressRange(gsl::span<AudioSamplePack> buffer) override;
	};
}
#endif
