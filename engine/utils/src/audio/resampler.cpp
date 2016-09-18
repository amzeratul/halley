#include "halley/audio/resampler.h"
#include "../../contrib/speex/speex_resampler.h"

using namespace Halley;

AudioResampler::AudioResampler(int from, int to, int nChannels, float quality)
{
	
}

AudioResampler::~AudioResampler()
{
	
}

void AudioResampler::resampleInterleaved(gsl::span<const float> src, gsl::span<float> dst)
{
	
}

size_t AudioResampler::numOutputSamples(size_t numInputSamples) const
{
	return 0;
}
