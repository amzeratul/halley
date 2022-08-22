#include "audio_source_delay.h"
#include "../audio_mixer.h"
using namespace Halley;

AudioSourceDelay::AudioSourceDelay(std::unique_ptr<AudioSource> src, size_t delay)
	: src(std::move(src))
	, initialDelay(delay)
	, curDelay(delay)
{
}

uint8_t AudioSourceDelay::getNumberOfChannels() const
{
	return src->getNumberOfChannels();
}

bool AudioSourceDelay::getAudioData(size_t numSamples, AudioMultiChannelSamples dst)
{
	if (curDelay == 0) {
		return src->getAudioData(numSamples, dst);
	} else if (numSamples < curDelay) {
		curDelay -= numSamples;
		AudioMixer::zero(dst);
		return true;
	} else {
		AudioMixer::zeroRange(dst, getNumberOfChannels(), 0, curDelay);
		auto dst2 = dst;
		for (auto& c: dst2) {
			c = c.subspan(curDelay);
		}
		const auto playing = src->getAudioData(numSamples - curDelay, dst2);
		curDelay = 0;
		return playing;
	}
}

bool AudioSourceDelay::isReady() const
{
	return src->isReady();
}

size_t AudioSourceDelay::getSamplesLeft() const
{
	return curDelay + src->getSamplesLeft();
}

void AudioSourceDelay::restart()
{
	curDelay = initialDelay;
	src->restart();
}
