#include "audio_engine.h"
#include <cmath>
#include "audio_mixer.h"

using namespace Halley;

AudioEngine::AudioEngine()
	: mixer(AudioMixer::makeMixer())
	, running(true)
	, needsBuffer(true)
{
}

AudioEngine::~AudioEngine()
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

void AudioEngine::playWorld(std::shared_ptr<AudioClip> clip, Vector2f position, float volume)
{
	sources.push_back(std::make_unique<AudioSource>(clip, AudioSourcePosition::makePositional(Vector3f(position)), volume));
}

void AudioEngine::setListener(Vector2f position)
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

	needsBuffer = false;
	generateBuffer();
}

void AudioEngine::generateBuffer()
{
	updateSources();

	for (size_t i = 0; i < spec.numChannels; ++i) {
		mixChannel(i, channelBuffers[i].packs);
	}

	postUpdateSources();

	mixer->interleaveChannels(backBuffer, channelBuffers);
	out->lockOutputDevice();
	std::swap(backBuffer, frontBuffer);
	out->unlockOutputDevice();
}

void AudioEngine::serviceAudio(gsl::span<AudioSamplePack> buffer)
{
	needsBuffer = true;
	backBufferCondition.notify_one();
	memcpy(buffer.data(), frontBuffer.packs.data(), buffer.size_bytes());
}

void AudioEngine::start(AudioSpec s, AudioOutputAPI& o)
{
	spec = s;
	out = &o;

	const size_t bufferSize = spec.bufferSize / 16;
	backBuffer.packs.resize(bufferSize * spec.numChannels);
	frontBuffer.packs.resize(bufferSize * spec.numChannels);

	tmpBuffer.packs.resize(bufferSize);
	channelBuffers.resize(spec.numChannels);
	for (auto& b: channelBuffers) {
		b.packs.resize(bufferSize);
	}

	channels.resize(spec.numChannels);
	channels[0].pan = 0.0f;
	channels[1].pan = 1.0f;
}

void AudioEngine::stop()
{
	running = false;
	needsBuffer = false;
	backBufferCondition.notify_one();
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

void AudioEngine::postUpdateSources()
{
	for (auto& source : sources) {
		if (source->isPlaying()) {
			source->advancePlayback(backBuffer.packs.size() * 16 / spec.numChannels);
		}
	}
}

void AudioEngine::mixChannel(size_t channelNum, gsl::span<AudioSamplePack> dst)
{
	for (auto& pack : dst) {
		for (size_t i = 0; i < 16; ++i) {
			pack.samples[i] = 0.0f;
		}
	}
	for (auto& source : sources) {
		if (source->isPlaying()) {
			size_t nChannels = source->getNumberOfChannels();
			for (size_t i = 0; i < nChannels; ++i) {
				source->mixToBuffer(i, channelNum, tmpBuffer.packs, dst, *mixer);
			}
		}
	}
}
