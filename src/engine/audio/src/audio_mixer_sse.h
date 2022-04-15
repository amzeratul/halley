#pragma once
#include "audio_mixer.h"

#ifdef HAS_SSE
namespace Halley
{
	class AudioMixerSSE final : public AudioMixer
	{
	public:
		void mixAudio(AudioSamplesConst src, AudioSamples dst, float gainStart, float gainEnd) override;
		void compressRange(AudioSamples buffer) override;
	};
}
#endif
