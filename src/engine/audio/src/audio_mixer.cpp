#include "audio_mixer.h"
#include "halley/utils/utils.h"
#include "audio_mixer_sse.h"
#include "audio_mixer_avx.h"

using namespace Halley;

void AudioMixer::mixAudio(gsl::span<const AudioSamplePack> src, gsl::span<AudioSamplePack> dst, float gain0, float gain1)
{
	const size_t nSamples = size_t(src.size());

	if (std::abs(gain0 - gain1) < 0.0001f) {
		// If the gain doesn't change, the code is faster
		if (std::abs(gain0 - 1.0f) < 0.0001f) {
			// No need to even multiply
			for (size_t i = 0; i < nSamples; ++i) {
				dst[i] += src[i];
			}
		} else {
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

void AudioMixer::interleaveChannels(gsl::span<AudioSamplePack> dstBuffer, gsl::span<AudioBuffer*> srcs)
{
	const size_t nChannels = srcs.size();	
	const size_t nSamples = dstBuffer.size() / nChannels;
	for (size_t i = 0; i < nSamples; ++i) {
		for (size_t j = 0; j < nChannels; ++j) {
			dstBuffer[i * nChannels + j] = srcs[j]->samples[i];
		}
	}
}

void AudioMixer::concatenateChannels(gsl::span<AudioSamplePack> dst, gsl::span<AudioBuffer*> srcs)
{
	size_t pos = 0;
	for (size_t i = 0; i < size_t(srcs.size()); ++i) {
		const size_t nBytes = srcs[i]->samples.size() * sizeof(AudioSamplePack);
		memcpy(dst.subspan(pos, nBytes).data(), srcs[i]->samples.data(), nBytes);
		pos += nBytes;
	}
}

void AudioMixer::compressRange(gsl::span<AudioSamplePack> buffer)
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

void AudioMixer::copy(AudioMultiChannelSamples src, std::array<gsl::span<AudioConfig::SampleFormat>, AudioConfig::maxChannels> dst, size_t nChannels)
{
	const size_t n = std::min(nChannels, std::min(src.size(), dst.size()));
	for (size_t c = 0; c < n; ++c) {
		copy(src[c], dst[c]);
	}
}

void AudioMixer::copy(AudioChannelSamples src, gsl::span<AudioConfig::SampleFormat> dst)
{
	memcpy(dst.data(), src.data(), std::min(src.size(), dst.size()) * sizeof(AudioConfig::SampleFormat));
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

std::unique_ptr<AudioMixer> AudioMixer::makeMixer()
{
	return std::make_unique<AudioMixer>();

	/*
#ifdef HAS_AVX
	if (hasAVX()) {
		return std::make_unique<AudioMixerAVX>();
	} else {
		return std::make_unique<AudioMixerSSE>();
	}
#elif HAS_SEE
	return std::make_unique<AudioMixerSSE>();
#else
	return std::make_unique<AudioMixer>();
#endif
	*/
}
