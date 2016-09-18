#include "halley/audio/resampler.h"
#include "../../contrib/speex/speex_resampler.h"

using namespace Halley;

static std::unique_ptr<SpeexResamplerState, void(*)(SpeexResamplerState*)> makeResampler(int from, int to, int channels, float quality)
{
	int err;
	auto * rawResampler = speex_resampler_init(unsigned(channels), unsigned(from), unsigned(to), std::max(0, std::min(int(quality * 10 + 0.5f), 10)), &err);
	return std::unique_ptr<SpeexResamplerState, void(*)(SpeexResamplerState*)>(rawResampler, [] (SpeexResamplerState* s) { speex_resampler_destroy(s); });
}

AudioResampler::AudioResampler(int from, int to, int nChannels, float quality)
	: resampler(makeResampler(from, to, nChannels, quality))
	, nChannels(size_t(nChannels))
	, from(from)
	, to(to)
{
}

AudioResampler::~AudioResampler() = default;

AudioResamplerResult AudioResampler::resampleInterleaved(gsl::span<const float> src, gsl::span<float> dst)
{
	unsigned inLen = unsigned(src.size() / nChannels);
	unsigned outLen = unsigned(dst.size() / nChannels);
	speex_resampler_process_interleaved_float(resampler.get(), src.data(), &inLen, dst.data(), &outLen);
	AudioResamplerResult result;
	result.nRead = inLen;
	result.nWritten = outLen;
	return result;
}

size_t AudioResampler::numOutputSamples(size_t numInputSamples) const
{
	return numInputSamples * to / from;
}
