#pragma once
#include "audio_mixer.h"

#ifdef HAS_AVX
namespace Halley
{
	class AudioMixerAVX final : public AudioMixer
	{
	public:
		void mixAudio(AudioSamplesConst src, AudioSamples dst, float gainStart, float gainEnd) override;
		void compressRange(AudioSamples buffer) override;
	};
}
#endif
