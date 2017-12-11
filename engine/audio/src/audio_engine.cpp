#include "audio_engine.h"
#include "audio_mixer.h"
#include <thread>
#include <chrono>
#include "audio_source_clip.h"
#include "audio_filter_resample.h"

using namespace Halley;

AudioEngine::AudioEngine()
	: mixer(AudioMixer::makeMixer())
	, pool(std::make_unique<AudioBufferPool>())
	, running(true)
	, needsBuffer(true)
{
}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::play(size_t id, std::shared_ptr<const AudioClip> clip, AudioPosition position, float volume, bool loop, float pitch)
{
	std::shared_ptr<AudioSource> source = std::make_shared<AudioSourceClip>(clip, loop);
	if (std::abs(pitch - 1.0f) > 0.01f) {
		pitch = clamp(pitch, 0.1f, 2.0f);
		source = std::make_shared<AudioFilterResample>(source, int(48000 * pitch + 0.5f), 48000, *pool);
	}
	addEmitter(id, std::make_unique<AudioEmitter>(source, position, volume));
}

void AudioEngine::setListener(AudioListenerData l)
{
	listener = l;
}

void AudioEngine::run()
{
	//const size_t bufSize = spec.numChannels * sizeof(AudioConfig::SampleFormat) * spec.bufferSize;

	// Generate one buffer
	if (running && out->needsMoreAudio()) {
		generateBuffer();
	}

	// OK, we've supplied it with enough buffers; if that was enough, then, sleep as long as no more buffers are needed
	while (running && !out->needsMoreAudio()) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100us);
	}
	
	// When we get here, it means that buffers are needed again (either one wasn't enough, or we waited long enough),
	// but first return so we the AudioFacade can update the incoming sound data
}

void AudioEngine::addEmitter(size_t id, std::unique_ptr<AudioEmitter>&& src)
{
	emitters.emplace_back(std::move(src));
	emitters.back()->setId(id);
	idToSource[id] = emitters.back().get();
}

AudioEmitter* AudioEngine::getSource(size_t id)
{
	auto src = idToSource.find(id);
	if (src != idToSource.end()) {
		return src->second;
	} else {
		return nullptr;
	}
}

std::vector<size_t> AudioEngine::getPlayingSounds()
{
	std::vector<size_t> result(idToSource.size());
	size_t i = 0;
	for (auto& kv: idToSource) {
		result[i++] = kv.first;
	}
	return result;
}

void AudioEngine::start(AudioSpec s, AudioOutputAPI& o)
{
	spec = s;
	out = &o;

	const size_t bufferSize = spec.bufferSize / 16;

	channelBuffers.resize(spec.numChannels);
	for (auto& b: channelBuffers) {
		b.packs.resize(bufferSize);
	}

	channels.resize(spec.numChannels);
	channels[0].pan = -1.0f;
	channels[1].pan = 1.0f;

	if (spec.sampleRate != 48000) {
		outResampler = std::make_unique<AudioResampler>(48000, spec.sampleRate, spec.numChannels, 0.5f);
	}
}

void AudioEngine::stop()
{
	running = false;
	needsBuffer = false;
}

void AudioEngine::generateBuffer()
{
	const size_t samplesToRead = spec.bufferSize;

	mixEmitters();
	removeFinishedEmitters();

	auto buffer = pool->getBuffer(samplesToRead * spec.numChannels);
	mixer->interleaveChannels(buffer.getBuffer(), channelBuffers);
	mixer->compressRange(buffer.getSpan());

	// Resample to output sample rate, if necessary
	if (outResampler) {
		auto resampledBuffer = pool->getBuffer(samplesToRead * spec.numChannels * spec.sampleRate / 48000 + 16);
		auto result = outResampler->resampleInterleaved(buffer.getSampleSpan(), resampledBuffer.getSampleSpan());
		if (result.nRead != samplesToRead) {
			throw Exception("Failed to read all input sample data");
		}
		out->queueAudio(resampledBuffer.getSampleSpan().subspan(0, result.nWritten * spec.numChannels));
	} else {
		out->queueAudio(buffer.getSampleSpan());
	}
}

void AudioEngine::mixEmitters()
{
	// Clear buffers
	for (size_t i = 0; i < spec.numChannels; ++i) {
		clearBuffer(channelBuffers[i].packs);
	}

	// Mix every emitter
	for (auto& e: emitters) {
		// Start playing if necessary
		if (!e->isPlaying() && !e->isDone() && e->isReady()) {
			e->start();
		}

		// Mix it in!
		if (e->isPlaying()) {
			e->update(channels, listener);
			e->mixTo(channelBuffers, *mixer, *pool);
		}
	}
}

void AudioEngine::removeFinishedEmitters()
{
	for (auto& e: emitters) {
		if (e->isDone()) {
			idToSource.erase(e->getId());
		}
	}
	emitters.erase(std::remove_if(emitters.begin(), emitters.end(), [&] (const std::unique_ptr<AudioEmitter>& src) { return src->isDone(); }), emitters.end());
}

void AudioEngine::clearBuffer(gsl::span<AudioSamplePack> dst)
{
	for (auto& pack : dst) {
		for (size_t i = 0; i < AudioSamplePack::NumSamples; ++i) {
			pack.samples[i] = 0.0f;
		}
	}
}
