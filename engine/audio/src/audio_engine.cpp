#include "audio_engine.h"
#include <cmath>

using namespace Halley;

AudioEngine::AudioEngine()
	: running(true)
	, needsBuffer(true)
{
}

AudioCallback AudioEngine::getCallback()
{
	return [this] (gsl::span<AudioSamplePack> dst) { serviceAudio(dst); };
}

void AudioEngine::playUI(std::shared_ptr<AudioClip> clip, float volume, float pan)
{
	// TODO
}

void AudioEngine::run()
{
	{
		std::unique_lock<std::mutex> lock(mutex);
		while (!needsBuffer && running) {
			backBufferCondition.wait(lock);
		}
	}

	if (!running) {
		return;
	}

	generateBuffer();
}

void AudioEngine::start(AudioSpec s)
{
	spec = s;
	backBuffer.samples.resize(spec.bufferSize * spec.numChannels / 16);
}

void AudioEngine::stop()
{
	running = false;
	needsBuffer = false;
	backBufferCondition.notify_one();
}

void AudioEngine::serviceAudio(gsl::span<AudioSamplePack> buffer)
{
	Expects(buffer.size() == backBuffer.samples.size());

	for (ptrdiff_t i = 0; i < buffer.size(); ++i) {
		gsl::span<const AudioConfig::SampleFormat> src = backBuffer.samples[i].samples;
		gsl::span<AudioConfig::SampleFormat> dst = buffer[i].samples;
		for (size_t j = 0; j < 16; ++j) {
			dst[j] = src[j];
		}
	}

	needsBuffer = true;
	backBufferCondition.notify_one();
}

void AudioEngine::generateBuffer()
{
	// TODO: make this actually work instead of generating a sine wave

	constexpr float scale = 6.283185307179586476925286766559f / 256.0f;
	
	for (size_t i = 0; i < backBuffer.samples.size(); ++i) {
		gsl::span<AudioConfig::SampleFormat> dst = backBuffer.samples[i].samples;
		for (size_t j = 0; j < 16; j += 2) {
			size_t pos = j + i * 16;
			float amplitude = 0.25f * ::sin(pos * scale);
			dst[j] = amplitude;
			dst[j + 1] = amplitude;
		}
	}
}
