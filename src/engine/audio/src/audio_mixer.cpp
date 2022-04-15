#include "audio_mixer.h"
#include "halley/utils/utils.h"

using namespace Halley;


#if defined(_M_X64) || defined(__x86_64__)
#define HAS_SSE
#if !defined(__linux__)
#define HAS_AVX
#endif
#endif

#if defined(_M_IX86) || defined(__i386)
// Might not be available, but do we really care about such old processors?
#define HAS_SSE
#endif

#ifdef HAS_AVX
#include <xmmintrin.h>
#ifdef __clang__
#include <immintrin.h>
#endif
#ifdef _MSC_VER
#include <intrin.h>
#endif
#endif

#ifdef HAS_SSE
#include <xmmintrin.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#endif


void AudioMixer::mixAudio(AudioSamplesConst src, AudioSamples dst, float gain0, float gain1)
{
	const auto nSamples = static_cast<size_t>(src.size());

	if (std::abs(gain0 - gain1) < 0.0001f) {
		// If the gain doesn't change, the code is faster
		if (std::abs(gain0 - 1.0f) < 0.0001f) {
			// No need to even multiply
			for (size_t i = 0; i < nSamples; ++i) {
				dst[i] += src[i];
			}
		} else if (std::abs(gain0) > 0.0001f) {
			for (size_t i = 0; i < nSamples; ++i) {
				dst[i] += src[i] * gain0;
			}
		}
	} else {
		// Interpolate the gain
		const float scale = 1.0f / nSamples;
		for (size_t i = 0; i < nSamples; ++i) {
			dst[i] += src[i] * lerp(gain0, gain1, i * scale);
		}
	}
}

void AudioMixer::mixAudio(AudioMultiChannelSamplesConst src, AudioMultiChannelSamples dst, float gainStart, float gainEnd)
{
	auto n = std::min(src.size(), dst.size());
	for (decltype(n) i = 0; i < n; ++i) {
		mixAudio(src[i], dst[i], gainStart, gainEnd);
	}
}

void AudioMixer::mixAudio(AudioMultiChannelSamples src, AudioMultiChannelSamples dst, float gainStart, float gainEnd)
{
	auto n = std::min(src.size(), dst.size());
	for (decltype(n) i = 0; i < n; ++i) {
		mixAudio(src[i], dst[i], gainStart, gainEnd);
	}
}

void AudioMixer::interleaveChannels(AudioSamples dstBuffer, gsl::span<AudioBuffer*> srcs)
{
	const size_t nChannels = srcs.size();	
	const size_t nSamples = dstBuffer.size() / nChannels;
	for (size_t i = 0; i < nSamples; ++i) {
		for (size_t j = 0; j < nChannels; ++j) {
			dstBuffer[i * nChannels + j] = srcs[j]->samples[i];
		}
	}
}

void AudioMixer::concatenateChannels(AudioSamples dst, gsl::span<AudioBuffer*> srcs)
{
	size_t pos = 0;
	for (size_t i = 0; i < size_t(srcs.size()); ++i) {
		const size_t nBytes = srcs[i]->samples.size() * sizeof(AudioSample);
		memcpy(dst.subspan(pos, nBytes).data(), srcs[i]->samples.data(), nBytes);
		pos += nBytes;
	}
}

void AudioMixer::compressRange(AudioSamples buffer)
{
	for (size_t i = 0; i < buffer.size(); ++i) {
		float& sample = buffer[i];
		sample = std::max(-0.99995f, std::min(sample, 0.99995f));
	}
}

void AudioMixer::zero(AudioMultiChannelSamples dst, size_t nChannels)
{
	const size_t n = std::min(nChannels, dst.size());
	for (size_t i = 0; i < n; ++i) {
		memset(dst[i].data(), 0, dst[i].size_bytes());
	}
}

void AudioMixer::zeroRange(AudioMultiChannelSamples dst, size_t nChannels, size_t start, size_t len)
{
	const size_t nCh = std::min(nChannels, dst.size());
	for (size_t i = 0; i < nCh; ++i) {
		memset(dst[i].data() + start, 0, std::min(dst[i].size() - start, len) * sizeof(AudioSample));
	}
}

void AudioMixer::copy(AudioMultiChannelSamples dst, AudioMultiChannelSamples src, size_t nChannels)
{
	const size_t n = std::min(nChannels, std::min(src.size(), dst.size()));
	for (size_t c = 0; c < n; ++c) {
		copy(dst[c], src[c]);
	}
}

void AudioMixer::copy(AudioSamples dst, AudioSamples src)
{
	memcpy(dst.data(), src.data(), std::min(src.size(), dst.size()) * sizeof(AudioSample));
}

#ifdef HAS_SSE

#ifdef _MSC_VER

#include <intrin.h>

#else

#include <cpuid.h>
static inline unsigned long long _xgetbv(unsigned int index){
	unsigned int eax, edx;
	__asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(index));
	return ((unsigned long long)edx << 32) | eax;
}
#define _XCR_XFEATURE_ENABLED_MASK 0

#endif

#endif

#ifdef HAS_AVX
static bool hasAVX()
{
#ifndef _MSC_VER
    // It's crashing on Linux :(
    return false;
#endif

#ifdef HAS_SSE
	int regs[4];
	int i = 1;

#ifdef _WIN32
	__cpuid(regs, i);
#else
	asm volatile
	("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
	: "a" (i), "c" (0));
	// ECX is set to zero for CPUID function 4
#endif

	bool osUsesXSAVE_XRSTORE = regs[2] & (1 << 27) || false;
	bool cpuAVXSuport = regs[2] & (1 << 28) || false;

	if (osUsesXSAVE_XRSTORE && cpuAVXSuport) {
		unsigned long long xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
		return (xcrFeatureMask & 0x6) == 0x6;
	} else {
		return false;
	}
#else
	return false;
#endif
}
#endif


/*
#ifdef HAS_AVX
void AudioMixerAVX::mixAudio(AudioSamplesConst srcRaw, AudioSamples dstRaw, float gain0, float gain1)
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
		const float sc = 1.0f / (dst.size() * 16);
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

void AudioMixerAVX::compressRange(AudioSamples buffer)
{
	gsl::span<__m256> dst(reinterpret_cast<__m256*>(buffer.data()), buffer.size() * 2);
	const size_t nSamples = size_t(dst.size());

	float val = 0.99995f;
	__m256 minVal = { -val, -val, -val, -val, -val, -val, -val, -val };
	__m256 maxVal = { val, val, val, val, val, val, val, val };

	for (size_t i = 0; i < nSamples; ++i) {
		dst[i] = _mm256_max_ps(minVal, _mm256_min_ps(dst[i], maxVal));
	}
}

#endif

#ifdef HAS_SSE

void AudioMixerSSE::mixAudio(AudioSamplesConst srcRaw, AudioSamples dstRaw, float gain0, float gain1)
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

void AudioMixerSSE::compressRange(AudioSamples buffer)
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
*/
