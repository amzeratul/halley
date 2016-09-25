#include "audio_engine.h"
#include "audio_mixer.h"
#include <thread>
#include <chrono>

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

void AudioEngine::playUI(size_t id, std::shared_ptr<AudioClip> clip, float volume, float pan, bool loop)
{
	addSource(id, std::make_unique<AudioSource>(clip, AudioSourcePosition::makeUI(pan), volume, loop));
}

void AudioEngine::playWorld(size_t id, std::shared_ptr<AudioClip> clip, Vector2f position, float volume, bool loop)
{
	addSource(id, std::make_unique<AudioSource>(clip, AudioSourcePosition::makePositional(Vector3f(position)), volume, loop));
}

void AudioEngine::setListener(Vector2f position)
{
	// TODO
}

void AudioEngine::run()
{
	const size_t bufSize = spec.numChannels * sizeof(AudioConfig::SampleFormat) * spec.bufferSize;
	const size_t minQueue = bufSize * 2;

	using namespace std::chrono_literals;
	while (out->getQueuedSize() >= minQueue && running) {
		std::this_thread::sleep_for(100us);
	}

	if (!running) {
		return;
	}

	while (out->getQueuedSize() < minQueue && running) {
		generateBuffer();
	}
}

void AudioEngine::addSource(size_t id, std::unique_ptr<AudioSource>&& src)
{
	sources.emplace_back(std::move(src));
	sources.back()->setId(id);
	idToSource[id] = sources.back().get();
}

AudioSource* AudioEngine::getSource(size_t id)
{
	auto src = idToSource.find(id);
	if (src != idToSource.end()) {
		return src->second;
	} else {
		return nullptr;
	}
}

void AudioEngine::generateBuffer()
{
	updateSources();

	for (size_t i = 0; i < spec.numChannels; ++i) {
		mixChannel(i, channelBuffers[i].packs);
	}

	postUpdateSources();

	mixer->interleaveChannels(backBuffer, channelBuffers);
	out->queueAudio(backBuffer.packs);
}

void AudioEngine::start(AudioSpec s, AudioOutputAPI& o)
{
	spec = s;
	out = &o;

	const size_t bufferSize = spec.bufferSize / 16;
	backBuffer.packs.resize(bufferSize * spec.numChannels);

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
			// Remove source
			if (sources.size() > 1) {
				std::swap(sources[i], sources.back());
			}
			idToSource.erase(sources.back()->getId());
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
