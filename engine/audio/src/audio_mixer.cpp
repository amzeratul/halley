#include "audio_mixer.h"
#include "halley/utils/utils.h"

using namespace Halley;

#define HAS_SSE

#ifdef HAS_SSE

#include <xmmintrin.h>

static void mixAudioAVX(gsl::span<const AudioSamplePack> srcRaw, gsl::span<AudioSamplePack> dstRaw, float gain0, float gain1)
{
	gsl::span<const __m256> src(reinterpret_cast<const __m256*>(srcRaw.data()), srcRaw.size() * 2);
	gsl::span<__m256> dst(reinterpret_cast<__m256*>(dstRaw.data()), dstRaw.size() * 2);
	const size_t nSamples = size_t(src.size());

	if (gain0 == gain1) {
		__m256 gain = { gain0, gain0, gain0, gain0, gain0, gain0, gain0, gain0 };
		for (size_t i = 0; i < nSamples; i += 2) {
			dst[i] = _mm256_add_ps(dst[i], _mm256_mul_ps(src[i], gain));
			dst[i + 1] = _mm256_add_ps(dst[i + 1], _mm256_mul_ps(src[i + 1], gain));
		}
	} else {
		const float sc = 1.0f / dst.size();
		const float gainDiff = gain1 - gain0;

		__m256 gain0p = { gain0, gain0, gain0, gain0, gain0, gain0, gain0, gain0 };
		__m256 gain1p = { gainDiff, gainDiff, gainDiff, gainDiff, gainDiff, gainDiff, gainDiff, gainDiff };
		__m256 scale = { sc, sc, sc, sc, sc, sc, sc, sc };
		__m256 offset = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };
		__m256 inc = { 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f };
		for (size_t i = 0; i < nSamples; ++i) {
			__m256 t = _mm256_mul_ps(offset, scale);
			__m256 gain = _mm256_add_ps(_mm256_mul_ps(gain1p, t), gain0p);
			offset = _mm256_add_ps(offset, inc);
			dst[i] = _mm256_add_ps(dst[i], _mm256_mul_ps(src[i], gain));
		}
	}
}

static void mixAudioSSE(gsl::span<const AudioSamplePack> srcRaw, gsl::span<AudioSamplePack> dstRaw, float gain0, float gain1)
{
	gsl::span<const __m128> src(reinterpret_cast<const __m128*>(srcRaw.data()), srcRaw.size() * 4);
	gsl::span<__m128> dst(reinterpret_cast<__m128*>(dstRaw.data()), dstRaw.size() * 4);
	const size_t nSamples = size_t(src.size());

	if (gain0 == gain1) {
		__m128 gain = { gain0, gain0, gain0, gain0 };
		for (size_t i = 0; i < nSamples; i += 4) {
			dst[i] = _mm_add_ps(dst[i], _mm_mul_ps(src[i], gain));
			dst[i + 1] = _mm_add_ps(dst[i + 1], _mm_mul_ps(src[i + 1], gain));
			dst[i + 2] = _mm_add_ps(dst[i + 2], _mm_mul_ps(src[i + 2], gain));
			dst[i + 3] = _mm_add_ps(dst[i + 3], _mm_mul_ps(src[i + 3], gain));
		}
	} else {
		const float sc = 1.0f / dst.size();
		const float gainDiff = gain1 - gain0;

		__m128 gain0p = { gain0, gain0, gain0, gain0 };
		__m128 gain1p = { gainDiff, gainDiff, gainDiff, gainDiff };
		__m128 scale = { sc, sc, sc, sc };
		__m128 offset = { 0.0f, 1.0f, 2.0f, 3.0f };
		__m128 inc = { 4.0f, 4.0f, 4.0f, 4.0f };
		for (size_t i = 0; i < nSamples; ++i) {
			__m128 t = _mm_mul_ps(offset, scale);
			__m128 gain = _mm_add_ps(_mm_mul_ps(gain1p, t), gain0p);
			offset = _mm_add_ps(offset, inc);
			dst[i] = _mm_add_ps(dst[i], _mm_mul_ps(src[i], gain));
		}
	}
}

#endif

static void mixAudioCpp(gsl::span<const AudioSamplePack> src, gsl::span<AudioSamplePack> dst, float gain0, float gain1)
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

static void interleaveChannelsCpp(AudioBuffer& dstBuffer, gsl::span<const AudioBuffer> src)
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

void AudioMixer::mixAudio(gsl::span<const AudioSamplePack> src, gsl::span<AudioSamplePack> dst, float gain0, float gain1)
{
#ifdef HAS_SSE
	mixAudioSSE(src, dst, gain0, gain1);
	//mixAudioAVX(src, dst, gain0, gain1);
#else
	mixAudioCpp(src, dst, gain0, gain1);
#endif
}

void AudioMixer::interleaveChannels(AudioBuffer& dstBuffer, gsl::span<const AudioBuffer> src)
{
	interleaveChannelsCpp(dstBuffer, src);
}
