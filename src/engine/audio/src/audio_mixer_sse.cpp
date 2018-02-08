#include "audio_mixer_sse.h"

#ifdef HAS_SSE
#include <xmmintrin.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

using namespace Halley;

void AudioMixerSSE::mixAudio(gsl::span<const AudioSamplePack> srcRaw, gsl::span<AudioSamplePack> dstRaw, float gain0, float gain1)
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
		const float sc = 1.0f / (dst.size() * 16);
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

void AudioMixerSSE::compressRange(gsl::span<AudioSamplePack> buffer)
{
	gsl::span<__m128> dst(reinterpret_cast<__m128*>(buffer.data()), buffer.size() * 4);
	const size_t nSamples = size_t(dst.size());

	float val = 0.99995f;
	__m128 minVal = {-val, -val, -val, -val};
	__m128 maxVal = {val, val, val, val};

	for (size_t i = 0; i < nSamples; ++i) {
		dst[i] = _mm_max_ps(minVal, _mm_min_ps(dst[i], maxVal));
	}
}

#endif
