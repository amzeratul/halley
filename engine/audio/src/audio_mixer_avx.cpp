#include "audio_mixer_avx.h"

#ifdef HAS_SSE
#include <xmmintrin.h>
using namespace Halley;

void AudioMixerAVX::mixAudio(gsl::span<const AudioSamplePack> srcRaw, gsl::span<AudioSamplePack> dstRaw, float gain0, float gain1)
{
	gsl::span<const __m256> src(reinterpret_cast<const __m256*>(srcRaw.data()), srcRaw.size() * 2);
	gsl::span<__m256> dst(reinterpret_cast<__m256*>(dstRaw.data()), dstRaw.size() * 2);
	const size_t nSamples = size_t(src.size());

	if (gain0 == gain1) {
		__m256 gain = _mm256_broadcast_ss(&gain0);
		for (size_t i = 0; i < nSamples; i += 2) {
			dst[i] = _mm256_add_ps(dst[i], _mm256_mul_ps(src[i], gain));
			dst[i + 1] = _mm256_add_ps(dst[i + 1], _mm256_mul_ps(src[i + 1], gain));
		}
	} else {
		const float sc = 1.0f / dst.size();
		const float gainDiff = gain1 - gain0;
		const float eight = 8.0f;

		__m256 gain0p = _mm256_broadcast_ss(&gain0);
		__m256 gain1p = _mm256_broadcast_ss(&gainDiff);
		__m256 scale = _mm256_broadcast_ss(&sc);
		__m256 inc = _mm256_broadcast_ss(&eight);
		__m256 offset = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };
		for (size_t i = 0; i < nSamples; ++i) {
			__m256 t = _mm256_mul_ps(offset, scale);
			__m256 gain = _mm256_add_ps(gain0p, _mm256_mul_ps(gain1p, t));
			offset = _mm256_add_ps(offset, inc);
			dst[i] = _mm256_add_ps(dst[i], _mm256_mul_ps(src[i], gain));
		}
	}
}

#endif