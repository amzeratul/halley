#include "audio_clip_streaming.h"
#include "audio_mixer.h"

using namespace Halley;

AudioClipStreaming::AudioClipStreaming(uint8_t numChannels)
	: length(0)
	, samplesLeft(0)
	, numChannels(numChannels)
{
	buffers.resize(numChannels);
}

void AudioClipStreaming::addInterleavedSamples(AudioSamplesConst src)
{
	std::unique_lock<std::mutex> lock(mutex);

	const size_t nSamples = src.size() / numChannels;

	for (size_t i = 0; i < numChannels; ++i) {
		const size_t startSize = buffers[i].size();
		buffers[i].resize(startSize + nSamples);
		for (size_t j = 0; j < nSamples; ++j) {
			buffers[i][j + startSize] = src[i + j * numChannels];
		}
	}

	length += nSamples;
	samplesLeft += nSamples;
}

void AudioClipStreaming::addInterleavedSamplesWithResample(AudioSamplesConst src, float sourceSampleRate)
{
	if (!resampler) {
		resampler = std::make_unique<AudioResampler>(lroundl(sourceSampleRate), 48000, numChannels, 1.0f);
	}

	const auto nOut = resampler->numOutputSamples(src.size());
	const auto minBufferSize = nextPowerOf2(nOut + numChannels); // Not sure if the extra sample per channel is needed
	if (resampleAudioBuffer.size() < minBufferSize) {
		resampleAudioBuffer.resize(minBufferSize);
	}

	const auto dst = gsl::span<float>(resampleAudioBuffer.data(), nOut);
	resampler->resampleInterleaved(src, dst);

	addInterleavedSamples(dst);
}

size_t AudioClipStreaming::copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const
{
	std::unique_lock<std::mutex> lock(mutex);

	auto& buffer = buffers[channelN];
	const size_t toWrite = std::min(len, buffer.size());

	AudioMixer::copy(dst, buffer, gain0, gain1);
	buffer.erase(buffer.begin(), buffer.begin() + toWrite);

	if (toWrite < len) {
		AudioMixer::zero(dst.subspan(toWrite, len - toWrite));
	}

	samplesLeft -= toWrite;

	return len;
}

uint8_t AudioClipStreaming::getNumberOfChannels() const
{
	return numChannels;
}

size_t AudioClipStreaming::getLength() const
{
	return length;
}

size_t AudioClipStreaming::getSamplesLeft() const
{
	return samplesLeft;
}
