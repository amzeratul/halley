#include "halley/audio/audio_clip_streaming.h"
#include "audio_mixer.h"
#include "halley/api/core_api.h"
#include "halley/support/logger.h"
#include "halley/time/stopwatch.h"

using namespace Halley;

namespace {
	constexpr float outSampleRate = 48000.0f;
}

AudioClipStreaming::AudioClipStreaming(uint8_t numChannels)
	: length(0)
	, samplesLeft(0)
	, numChannels(numChannels)
	, latencyTarget(2048)
	, samplesLeftAvg(30)
{
	buffers.resize(numChannels, RingBuffer<float>(latencyTarget * 2));
}

AudioClipStreaming::~AudioClipStreaming()
{
	if (coreAPI) {
		coreAPI->removeStartFrameCallback(this);
	}
}

void AudioClipStreaming::addInterleavedSamples(AudioSamplesConst src)
{
	std::unique_lock<std::mutex> lock(mutex);

	std::array<float, 4096> tmp;
	//assert(src.size() / numChannels < tmp.size());
	const size_t nSamples = std::min(src.size() / numChannels, tmp.size());

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
		resampler = std::make_unique<AudioResampler>(sourceSampleRate, outSampleRate, numChannels, 0.0f);
	}

	resampler->setRate(sourceSampleRate, outSampleRate);

	doAddInterleavedSamplesWithResample(src);
}

void AudioClipStreaming::addInterleavedSamplesWithResampleSync(AudioSamplesConst src, float sourceSampleRate, float maxPitchShift, CoreAPI& core, AudioOutputAPI& audioOut)
{
	if (!coreAPI) {
		coreAPI = &core;
		coreAPI->addStartFrameCallback(this);
	}
	audioOutAPI = &audioOut;

	if (!resampler) {
		resampler = std::make_unique<AudioResampler>(sourceSampleRate, outSampleRate, numChannels, 0.0f);
	}

	this->sourceSampleRate = sourceSampleRate;
	this->maxPitchShift = maxPitchShift;
	doAddInterleavedSamplesWithResample(src);

	//updateSync(maxPitchShift, sourceSampleRate);
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
	}
	lock.unlock();

	std::array<float, 4096> tmp;
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

size_t AudioClipStreaming::getTargetSamples() const
{
	return clamp<size_t>(samplesLeft.load(), 256, 2048);
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
	const auto minBufferSize = nextPowerOf2(nOut + numChannels * 2); // Not sure if the extra samples per channel are needed
	if (resampleDstBuffer.size() < minBufferSize) {
		resampleDstBuffer.resize(minBufferSize);
	}

	const auto dst = resampleDstBuffer.span();
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

void AudioClipStreaming::onStartFrame()
{
	// Based on paper
	// "Dynamic Rate Control for Retro Game Emulators"
	// By Hans-Kristian Arntzen
	// December 12, 2012
	// https://raw.githubusercontent.com/libretro/docs/master/archive/ratecontrol.pdf

	const int64_t sentSamplesLeft = audioOutAPI->getSamplesLeft();//static_cast<int64_t>(lastSamplesSent) - consumedSinceLastSubmission;
	const int64_t bufferedSamplesLeft = static_cast<int64_t>(buffers[0].availableToRead());
	const int64_t playbackSamplesLeft = sentSamplesLeft + bufferedSamplesLeft;

	samplesLeftAvg.add(playbackSamplesLeft);

	if (samplesLeftAvg.size() >= 3) {
		const float d = maxPitchShift;
		const float avgSamplesLeft = samplesLeftAvg.getFloatMean();

		const auto AB = latencyTarget * 2;
		const float ratio = AB / ((1 + d) * AB - 2.0f * d * avgSamplesLeft);
		const auto fromRate = sourceSampleRate * ratio;

		//Logger::logDev(toString(int(playbackSamplesLeft), 10, 4, ' ') + " [" + toString(int(avgSamplesLeft), 10, 4, ' ') + "], " + toString(ratio, 4) + "x");
		resampler->setRate(fromRate, outSampleRate);
	} else {
		resampler->setRate(sourceSampleRate, outSampleRate);
	}

	/*
	const auto now = std::chrono::high_resolution_clock::now();
	const auto samplesPlayedNow = audioOutAPI->getSamplesPlayed();
	if (lastFrameStartTime) {
		const float elapsedSinceLastFrame = static_cast<float>(std::chrono::duration(now - *lastFrameStartTime).count()) / 1'000'000'000.0f;
		const int64_t playedSinceLastFrame = static_cast<int64_t>(samplesPlayedNow - lastFrameSamplesPlayed);
		const int64_t submittedLastFrame = static_cast<int64_t>(lastFrameSamples);
		const float timeRatio = lastFrameTimeLength / elapsedSinceLastFrame;
		const float sampleRatio = static_cast<float>(submittedLastFrame) / static_cast<float>(playedSinceLastFrame);
		const float overallRatio = sampleRatio / timeRatio;
		
		//Logger::logDev("+" + toString(submittedLastFrame) + " (" + toString(lroundl(lastFrameTimeLength * 1'000'000.0)) + "us) | -" + toString(playedSinceLastFrame) + " (" + toString(elapsedSinceLastFrame / 1000) + " us)");
		Logger::logDev("Time: " + toString(timeRatio) + ", Samples: " + toString(sampleRatio) + ", Overall: " + toString(overallRatio));
	}
	// Setup for next frame
	lastFrameStartTime = now;
	lastFrameSamplesPlayed = samplesPlayedNow;
	lastFrameSamples = 0;
	*/
}
