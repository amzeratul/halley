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
	sources.push_back(std::make_unique<AudioSource>(clip, AudioSourcePosition::makeUI(pan), volume));
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
	updateSources();

	for (size_t i = 0; i < spec.numChannels; ++i) {
		mixChannel(i, nullptr);
	}

	for (size_t i = 0; i < backBuffer.samples.size(); ++i) {
		gsl::span<AudioConfig::SampleFormat> dst = backBuffer.samples[i].samples;
		for (size_t j = 0; j < 16; j += 2) {
			size_t pos = j + i * 16;
			dst[j] = 0;
			dst[j + 1] = 0;
		}
	}
}

void AudioEngine::updateSources()
{
	size_t n = sources.size();
	for (size_t i = 0; i < n; ++i) {
		if (sources[i]->isPlaying()) {
			sources[i]->update(channels);
		} else if (sources[i]->isDone()) {
			if (sources.size() > 1) {
				std::swap(sources[i], sources.back());
			}
			sources.pop_back();
			--i;
			--n;
		} else if (sources[i]->isReady()) {
			sources[i]->start();
			sources[i]->update(channels);
		}
	}
}

void AudioEngine::mixChannel(size_t channelNum, gsl::span<AudioSamplePack> dst)
{
	for (auto& source : sources) {
		if (source->isPlaying()) {
			source->getChannelSamples(0, channelNum, tmp);
		}
	}
}
