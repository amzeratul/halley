#include "audio_mixer.h"
#include "halley/utils/utils.h"
#include "audio_mixer_sse.h"
#include "audio_mixer_avx.h"

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

void AudioMixer::interleaveChannels(AudioBuffer& dstBuffer, gsl::span<const AudioBuffer> src)
{
	size_t n = 0;
	for (size_t i = 0; i < dstBuffer.packs.size(); ++i) {
		gsl::span<AudioConfig::SampleFormat> dst = dstBuffer.packs[i].samples;
		size_t srcIdx = i >> 1;
		size_t srcOff = (i & 1) << 3;

		for (size_t j = 0; j < 8; ++j) {
			size_t srcPos = j + srcOff;
			dst[2 * j] = src[0].packs[srcIdx].samples[srcPos];
			dst[2 * j + 1] = src[1].packs[srcIdx].samples[srcPos];
			n += 2;
		}
	}
}

std::unique_ptr<AudioMixer> AudioMixer::makeMixer()
{
#ifdef HAS_SSE
	bool hasAvx = false;
	if (hasAvx) {
		return std::make_unique<AudioMixerAVX>();
	} else {
		return std::make_unique<AudioMixerSSE>();
	}
#else
	return std::make_unique<AudioMixer>();
#endif
}
