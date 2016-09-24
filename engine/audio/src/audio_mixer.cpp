#include "audio_mixer.h"
#include "halley/utils/utils.h"

using namespace Halley;

void AudioMixer::mixAudio(gsl::span<const AudioSamplePack> src, gsl::span<AudioSamplePack> dst, float gain0, float gain1)
{
	const size_t nPacks = size_t(src.size());

	if (gain0 == gain1) {
		// If the gain doesn't change, the code is faster
		for (size_t i = 0; i < nPacks; ++i) {
			for (size_t j = 0; j < 16; ++j) {
				dst[i].samples[j] += src[i].samples[j] * gain0;
			}
		}
	} else {
		// Interpolate the gain
		const float scale = 1.0f / dst.size();
		for (size_t i = 0; i < nPacks; ++i) {
			for (size_t j = 0; j < 16; ++j) {
				dst[i].samples[j] += src[i].samples[j] * lerp(gain0, gain1, (i * 16 + j) * scale);
			}
		}
	}
}
