#include "audio_clip_streaming.h"
#include "audio_mixer.h"
#include "halley/support/logger.h"

using namespace Halley;

namespace {
	constexpr int outSampleRate = 48000;
	const size_t maxBufferSize = 2048;
}

AudioClipStreaming::AudioClipStreaming(uint8_t numChannels)
	: length(0)
	, samplesLeft(0)
	, numChannels(numChannels)
	, samplesLeftAvg(8)
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

	if (samplesLeft >= maxBufferSize / 2) {
		ready = true;
	}
}

void AudioClipStreaming::addInterleavedSamplesWithResample(AudioSamplesConst src, float sourceSampleRate)
{
	if (!resampler) {
		resampler = std::make_unique<AudioResampler>(lroundl(sourceSampleRate), outSampleRate, numChannels, 1.0f);
	}

	resampler->setFromHz(lroundl(sourceSampleRate));

	doAddInterleavedSamplesWithResample(src);
}

void AudioClipStreaming::addInterleavedSamplesWithResampleSync(AudioSamplesConst src, float sourceSampleRate, float maxPitchShift)
{
	if (!resampler) {
		resampler = std::make_unique<AudioResampler>(lroundl(sourceSampleRate), outSampleRate, numChannels, 1.0f);
	}

	updateSync(maxPitchShift, sourceSampleRate);

	doAddInterleavedSamplesWithResample(src);
}

size_t AudioClipStreaming::copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const
{
	std::unique_lock<std::mutex> lock(mutex);

	auto& buffer = buffers[channelN];
	const size_t toWrite = std::min(len, buffer.size());

	AudioMixer::copy(dst, buffer, gain0, gain1);
	buffer.erase(buffer.begin(), buffer.begin() + toWrite);

	if (toWrite < len) {
		Logger::logWarning("AudioClipStreaming ran out of samples - had " + toString(static_cast<int>(toWrite)) + ", requested " + toString(static_cast<int>(len)));
		AudioMixer::zero(dst.subspan(toWrite, len - toWrite));
	}

	if (channelN == 0) {
		samplesLeft -= toWrite;
	}

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

bool AudioClipStreaming::isLoaded() const
{
	return ready;
}

void AudioClipStreaming::doAddInterleavedSamplesWithResample(AudioSamplesConst src)
{
	const auto nOut = resampler->numOutputSamples(src.size());
	const auto minBufferSize = nextPowerOf2(nOut + numChannels); // Not sure if the extra sample per channel is needed
	if (resampleAudioBuffer.size() < minBufferSize) {
		resampleAudioBuffer.resize(minBufferSize);
	}

	const auto dst = gsl::span<float>(resampleAudioBuffer.data(), nOut);
	resampler->resampleInterleaved(src, dst);

	addInterleavedSamples(dst);
}

void AudioClipStreaming::updateSync(float maxPitchShift, float sourceSampleRate)
{
	// Based on paper
	// "Dynamic Rate Control for Retro Game Emulators"
	// By Hans-Kristian Arntzen
	// December 12, 2012
	// https://raw.githubusercontent.com/libretro/docs/master/archive/ratecontrol.pdf

	const auto playbackSamplesLeft = samplesLeft.load();
	samplesLeftAvg.add(playbackSamplesLeft);

	if (samplesLeftAvg.size() >= 3) {
		const float d = maxPitchShift;
		const float avgSamplesLeft = samplesLeftAvg.getFloatMean();
		const float ratio = 1.0f / lerp(1.0f + d, 1.0f - d, clamp(avgSamplesLeft / static_cast<float>(maxBufferSize), 0.0f, 1.0f));
		const int fromRate = lroundl(sourceSampleRate * ratio);

		//Logger::logDev(toString(int(playbackSamplesLeft)) + " [" + toString(int(avgSamplesLeft)) + "], " + toString(ratio) + "x, " + toString(fromRate) + " Hz");
		resampler->setRate(fromRate, outSampleRate);
		//resampler->setRateFrac(lroundl(fromRate * 1000), 48'000'000, lroundl(sourceSampleRate), outSampleRate);
	} else {
		resampler->setRate(lroundl(sourceSampleRate), outSampleRate);
	}
}
