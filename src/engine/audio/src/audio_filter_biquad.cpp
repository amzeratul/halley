#include "audio_filter_biquad.h"

using namespace Halley;

AudioFilterBiquad::AudioFilterBiquad(std::shared_ptr<AudioSource> src)
	: src(std::move(src))
{
}

size_t AudioFilterBiquad::getNumberOfChannels() const
{
	return src->getNumberOfChannels();
}

bool AudioFilterBiquad::isReady() const
{
	return src->isReady();
}

bool AudioFilterBiquad::getAudioData(size_t numSamples, AudioSourceData& dst)
{
	// TODO
	return src->getAudioData(numSamples, dst);
}
