#pragma once
#include "audio_mixer.h"

#ifdef HAS_AVX
namespace Halley
{
	class AudioMixerAVX final : public AudioMixer
	{
	public:
		void mixAudio(gsl::span<const AudioSamplePack> src, gsl::span<AudioSamplePack> dst, float gainStart, float gainEnd) override;
		void compressRange(gsl::span<AudioSamplePack> buffer) override;
	};
}
#endif
