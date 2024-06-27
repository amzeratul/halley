#include "audio_filter_resample.h"
#include "halley/support/debug.h"

using namespace Halley;

AudioFilterResample::AudioFilterResample(std::shared_ptr<AudioSource> source, float fromHz, float toHz, AudioBufferPool& pool)
	: pool(pool)
	, source(std::move(source))
	, fromHz(fromHz)
	, toHz(toHz)
{
}

uint8_t AudioFilterResample::getNumberOfChannels() const
{
	return source->getNumberOfChannels();
}

bool AudioFilterResample::isReady() const
{
	return source->isReady();
}

bool AudioFilterResample::getAudioData(size_t numSamples, AudioMultiChannelSamples dstBuffers)
{
	const size_t nChannels = source->getNumberOfChannels();
	const size_t additionalPaddingSamples = 2;
	const size_t nLeftOver = leftoverSamples[0].n;
	const size_t samplesToGenerate = numSamples >= nLeftOver ? numSamples - nLeftOver : 0;
	const size_t numSamplesSrc = lroundl(samplesToGenerate * fromHz / toHz) + additionalPaddingSamples;

	if (resamplers.empty()) {
		for (size_t i = 0; i < nChannels; ++i) {
			resamplers.push_back(std::make_unique<AudioResampler>(fromHz, toHz, 1, 0.0f));
		}
	}

	// Read upstream data
	auto srcBuffers = pool.getBuffers(nChannels, numSamplesSrc);
	auto srcs = srcBuffers.getSampleSpans();
	bool playing = source->getAudioData(numSamplesSrc, srcs);

	// Prepare temporary destination data
	auto tmpBuffer = pool.getBuffer(numSamples + 32); // Is this +32 needed?
	auto tmp = tmpBuffer.getSpan();
	
	// Resample
	for (size_t channel = 0; channel < nChannels; ++channel) {
		// First copy any leftovers
		Expects(leftoverSamples[channel].n == nLeftOver);
		for (size_t i = 0; i < nLeftOver; ++i) {
			tmp[i] = leftoverSamples[channel].samples[i];
		}

		auto result = resamplers[channel]->resample(srcs[channel].subspan(0, numSamplesSrc), tmp.subspan(nLeftOver), 0);
		Expects(result.nRead == numSamplesSrc);
		Expects(result.nWritten >= samplesToGenerate);

		// Store left overs
		size_t leftOver = result.nWritten + nLeftOver - numSamples;
		leftOver = std::min(leftOver, leftoverSamples[channel].samples.size());
		for (size_t i = 0; i < leftOver; ++i) {
			leftoverSamples[channel].samples[i] = tmp[i + numSamples];
		}
		leftoverSamples[channel].n = leftOver;

		// Copy to destination
		memcpy(dstBuffers[channel].data(), tmp.data(), numSamples * sizeof(AudioSample));
	}

	return playing;
}

size_t AudioFilterResample::getSamplesLeft() const
{
	return lroundl(source->getSamplesLeft() * toHz / fromHz);
}

void AudioFilterResample::restart()
{
	source->restart();
	resamplers.clear();
}

void AudioFilterResample::setFromHz(float fromHz)
{
	this->fromHz = fromHz;
	for (auto& r: resamplers) {
		r->setFromHz(fromHz);
	}
}
