#include "halley/audio/resampler.h"
#include "../../../../contrib/speex/speex_resampler.h"
#include "halley/utils/utils.h"

using namespace Halley;

namespace {
	std::unique_ptr<SpeexResamplerState, void(*)(SpeexResamplerState*)> makeResampler(float from, float to, int channels, float quality)
	{
		int err;
		auto* rawResampler = speex_resampler_init(uint32_t(channels), uint32_t(from), uint32_t(to), std::max(0, std::min(int(quality * 10 + 0.5f), 10)), &err);
		return std::unique_ptr<SpeexResamplerState, void(*)(SpeexResamplerState*)>(rawResampler, [](SpeexResamplerState* s) { speex_resampler_destroy(s); });
	}
}

AudioResampler::AudioResampler(float from, float to, int nChannels, float quality)
	: resampler(makeResampler(from, to, nChannels, quality))
	, nChannels(size_t(nChannels))
	, from(from)
	, to(to)
{
}

AudioResampler::~AudioResampler() = default;

size_t AudioResampler::numOutputSamples(size_t numInputSamples) const
{
	return static_cast<size_t>(numInputSamples * to / from);
}

void AudioResampler::setFromHz(float from)
{
	this->from = from;
	speex_resampler_set_rate(resampler.get(), lroundl(from), lroundl(to));
}

void AudioResampler::setToHz(float to)
{
	this->to = to;
	speex_resampler_set_rate(resampler.get(), lroundl(from), lroundl(to));
}

void AudioResampler::setRate(float from, float to)
{
	this->from = from;
	this->to = to;
	speex_resampler_set_rate(resampler.get(), lroundl(from), lroundl(to));
}

AudioResamplerResult AudioResampler::resample(gsl::span<const float> src, gsl::span<float> dst, size_t channel)
{
	const auto origInLen = static_cast<uint32_t>(src.size() / nChannels);
	const auto origOutLen = static_cast<uint32_t>(dst.size() / nChannels);
	auto inLen = origInLen;
	auto outLen = origOutLen;

	speex_resampler_process_float(resampler.get(), uint32_t(channel), src.data(), &inLen, dst.data(), &outLen);

	AudioResamplerResult result;
	result.nRead = inLen;
	result.nWritten = outLen;
	return result;
}

AudioResamplerResult AudioResampler::resampleInterleaved(gsl::span<const float> src, gsl::span<float> dst)
{
	const auto origInLen = static_cast<uint32_t>(src.size() / nChannels);
	const auto origOutLen = static_cast<uint32_t>(dst.size() / nChannels);
	auto inLen = origInLen;
	auto outLen = origOutLen;

	speex_resampler_process_interleaved_float(resampler.get(), src.data(), &inLen, dst.data(), &outLen);

	AudioResamplerResult result;
	result.nRead = inLen;
	result.nWritten = outLen;
	return result;
}

AudioResamplerResult AudioResampler::resampleInterleaved(gsl::span<const short> src, gsl::span<short> dst)
{
	const auto origInLen = static_cast<uint32_t>(src.size() / nChannels);
	const auto origOutLen = static_cast<uint32_t>(dst.size() / nChannels);
	auto inLen = origInLen;
	auto outLen = origOutLen;

	speex_resampler_process_interleaved_int(resampler.get(), src.data(), &inLen, dst.data(), &outLen);

	AudioResamplerResult result;
	result.nRead = inLen;
	result.nWritten = outLen;
	return result;
}

AudioResamplerResult AudioResampler::resampleNonInterleaved(gsl::span<const float> src, gsl::span<float> dst, const size_t numChannels)
{
	AudioResamplerResult result;
	result.nRead = 0;
	result.nWritten = 0;

	for (size_t i = 0; i < numChannels; ++i) {
		uint32_t inLen = uint32_t(src.size() / nChannels);
		uint32_t outLen = uint32_t(dst.size() / nChannels);
		speex_resampler_process_float(resampler.get(), uint32_t(i), src.subspan(result.nRead).data(), &inLen, dst.subspan(result.nWritten).data(), &outLen);
		result.nRead += inLen;
		result.nWritten += outLen;
	}

	return result;
}
