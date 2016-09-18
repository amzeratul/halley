#include "audio_engine.h"

using namespace Halley;

AudioEngine::AudioEngine(AudioSpec spec)
	: spec(spec)
{
}

AudioCallback AudioEngine::getCallback()
{
	return [this] (gsl::span<AudioSamplePack> dst) { serviceAudio(dst); };
}

void AudioEngine::playUI(std::shared_ptr<AudioClip> clip, float volume, float pan)
{
	
}

void AudioEngine::serviceAudio(gsl::span<AudioSamplePack> buffer)
{
	// SIMD friendly
	for (ptrdiff_t i = 0; i < buffer.size(); ++i) {
		gsl::span<float> dst = buffer[i].samples;
		for (size_t j = 0; j < 16; ++j) {
			dst[j] = 0.0f;
		}
	}
}
