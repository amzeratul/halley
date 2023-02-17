#include "audio_clip_streaming.h"
#include "audio_mixer.h"
#include "halley/support/logger.h"

using namespace Halley;

namespace {
	constexpr float outSampleRate = 48000.0f;
}

AudioClipStreaming::AudioClipStreaming(uint8_t numChannels)
	: length(0)
	, samplesLeft(0)
	, numChannels(numChannels)
	, latencyTarget(2048)
	, samplesLeftAvg(20)
{
	buffers.resize(numChannels, RingBuffer<float>(latencyTarget * 2));
}

void AudioClipStreaming::addInterleavedSamples(AudioSamplesConst src)
{
	std::unique_lock<std::mutex> lock(mutex);

	const size_t nSamples = src.size() / numChannels;
	std::array<float, 2048> tmp;
	assert(nSamples < tmp.size());

	for (size_t i = 0; i < numChannels; ++i) {
		// For each channel, deinterleave
		auto srcSamples = src.data() + i;
		for (size_t j = 0; j < nSamples; ++j) {
			tmp[j] = *srcSamples;
			srcSamples += numChannels;
		}

		// Add to buffer
		const auto n = std::min(nSamples, buffers[i].availableToWrite());
		buffers[i].write(gsl::span<float>(tmp).subspan(0, n));
	}

	length += nSamples;
	samplesLeft += nSamples;

	if (samplesLeft >= latencyTarget) {
		ready = true;
	}
}

void AudioClipStreaming::addInterleavedSamplesWithResample(AudioSamplesConst src, float sourceSampleRate)
{
	if (!resampler) {
		resampler = std::make_unique<AudioResampler>(sourceSampleRate, outSampleRate, numChannels, 1.0f);
	}

	resampler->setRate(sourceSampleRate, outSampleRate);

	doAddInterleavedSamplesWithResample(src);
}

void AudioClipStreaming::addInterleavedSamplesWithResampleSync(AudioSamplesConst src, float sourceSampleRate, float maxPitchShift)
{
	if (!resampler) {
		resampler = std::make_unique<AudioResampler>(sourceSampleRate, outSampleRate, numChannels, 1.0f);
	}

	doAddInterleavedSamplesWithResample(src);

	updateSync(maxPitchShift, sourceSampleRate);
}

size_t AudioClipStreaming::copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const
{
	if (paused) {
		AudioMixer::zero(dst);
		return len;
	}

	auto& buffer = buffers[channelN];

	std::unique_lock<std::mutex> lock(mutex);
	const size_t toWrite = std::min(len, buffer.availableToRead());
	if (channelN == 0) {
		samplesLeft -= toWrite;
		lastSamplesSent = toWrite;
		lastSamplesSentTime = std::chrono::steady_clock::now();
	}
	lock.unlock();

	std::array<float, 2048> tmp;
	auto samples = gsl::span<float>(tmp).subspan(0, toWrite);
	buffer.read(samples);
	AudioMixer::copy(dst, samples, gain0, gain1);

	if (toWrite < len) {
		//Logger::logWarning("AudioClipStreaming ran out of samples - had " + toString(static_cast<int>(toWrite)) + ", requested " + toString(static_cast<int>(len)));
		AudioMixer::zero(dst.subspan(toWrite, len - toWrite));
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

void AudioClipStreaming::setLatencyTarget(size_t samples)
{
	latencyTarget = samples;
	buffers.clear();
	buffers.resize(numChannels, RingBuffer<float>(latencyTarget * 2));
	samplesLeft = 0;
}

size_t AudioClipStreaming::getLatencyTarget() const
{
	return latencyTarget;
}

void AudioClipStreaming::setPaused(bool paused)
{
	this->paused = paused;
}

void AudioClipStreaming::doAddInterleavedSamplesWithResample(AudioSamplesConst origSrc)
{
	AudioSamplesConst src;
	if (pending.empty()) {
		src = origSrc;
	} else {
		resampleSrcBuffer.resize(pending.size() + origSrc.size());
		memcpy(resampleSrcBuffer.data(), pending.data(), pending.size() * sizeof(float));
		memcpy(resampleSrcBuffer.data() + pending.size(), origSrc.data(), origSrc.size_bytes());
		pending.clear();
		src = resampleSrcBuffer;
	}

	const auto nOut = resampler->numOutputSamples(src.size());
	const auto minBufferSize = nextPowerOf2(nOut + numChannels); // Not sure if the extra sample per channel is needed
	if (resampleDstBuffer.size() < minBufferSize) {
		resampleDstBuffer.resize(minBufferSize);
	}

	const auto dst = gsl::span<float>(resampleDstBuffer.data(), nOut);
	const auto result = resampler->resampleInterleaved(src, dst);

	const size_t srcSamplesRead = result.nRead * numChannels;
	const size_t srcSamplesNotRead = src.size() - srcSamplesRead;
	if (srcSamplesNotRead > 0) {
		pending.resize(srcSamplesNotRead);
		memcpy(pending.data(), src.data() + srcSamplesRead, srcSamplesNotRead * sizeof(float));
	}
	resampleSrcBuffer.clear();

	addInterleavedSamples(dst.subspan(0, result.nWritten * numChannels));
}

void AudioClipStreaming::updateSync(float maxPitchShift, float sourceSampleRate)
{
	// Based on paper
	// "Dynamic Rate Control for Retro Game Emulators"
	// By Hans-Kristian Arntzen
	// December 12, 2012
	// https://raw.githubusercontent.com/libretro/docs/master/archive/ratecontrol.pdf

	const auto now = std::chrono::steady_clock::now();

	std::unique_lock<std::mutex> lock(mutex);
	const auto timeSinceLastSubmission = lastSamplesSent > 0 ? std::chrono::duration<double>(now - lastSamplesSentTime).count() : 0;
	const auto consumedSinceLastSubmission = static_cast<int64_t>(outSampleRate * timeSinceLastSubmission);
	const int64_t sentSamplesLeft = static_cast<int64_t>(lastSamplesSent) - consumedSinceLastSubmission;
	const int64_t bufferedSamplesLeft = static_cast<int64_t>(buffers[0].availableToRead());
	const int64_t playbackSamplesLeft = sentSamplesLeft + bufferedSamplesLeft;
	lock.unlock();

	samplesLeftAvg.add(playbackSamplesLeft);

	if (samplesLeftAvg.size() >= 3) {
		const float d = maxPitchShift;
		const float avgSamplesLeft = samplesLeftAvg.getFloatMean();
		const float ratio = 1.0f / lerp(1.0f + d, 1.0f - d, clamp(avgSamplesLeft / static_cast<float>(2 * latencyTarget), 0.0f, 1.0f));
		const auto fromRate = sourceSampleRate * ratio;

		//Logger::logDev(toString(int(playbackSamplesLeft)) + " = " + toString(bufferedSamplesLeft) + " + " + toString(sentSamplesLeft) + " [" + toString(int(avgSamplesLeft)) + "], " + toString(ratio) + "x, " + toString(fromRate) + " Hz");
		resampler->setRate(fromRate, outSampleRate);
	} else {
		resampler->setRate(sourceSampleRate, outSampleRate);
	}
}
