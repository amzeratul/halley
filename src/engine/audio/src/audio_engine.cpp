#include "audio_engine.h"
#include "audio_mixer.h"
#include <thread>
#include <chrono>
#include "audio_source_clip.h"
#include "audio_filter_resample.h"
#include "halley/support/debug.h"
#include "halley/core/resources/resources.h"
#include "audio_event.h"
#include "halley/support/logger.h"
#include "halley/core/api/audio_api.h"
#include "audio_variable_table.h"
#include "halley/support/profiler.h"
#include "halley/time/stopwatch.h"

using namespace Halley;

AudioEngine::AudioEngine()
	: mixer(AudioMixer::makeMixer())
	, pool(std::make_unique<AudioBufferPool>())
	, variableTable(std::make_unique<AudioVariableTable>())
	, audioOutputBuffer(4096 * 8)
	, running(true)
	, needsBuffer(true)
{
	rng.setSeed(Random::getGlobal().getRawInt());
}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::postEvent(uint32_t id, const AudioEvent& event, const AudioPosition& position)
{
	const size_t nEmitters = event.run(*this, id, position);
	if (nEmitters == 0) {
		finishedSounds.push_back(id);
	}
}

void AudioEngine::play(uint32_t id, std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop)
{
	addEmitter(id, std::make_unique<AudioVoice>(std::make_shared<AudioSourceClip>(std::move(clip), loop, 0), std::move(position), volume, getGroupId("")));
}

void AudioEngine::setListener(AudioListenerData l)
{
	listener = l;
}

void AudioEngine::setOutputChannels(std::vector<AudioChannelData> channelData)
{
	if (channels.size() == channelData.size()) {
		channels = std::move(channelData);
	}
}

void AudioEngine::run()
{
	//const size_t bufSize = spec.numChannels * sizeof(AudioConfig::SampleFormat) * spec.bufferSize;

	// Generate one buffer
	if (running && needsMoreAudio()) {
		generateBuffer();
	}

	// OK, we've supplied it with enough buffers; if that was enough, then, sleep as long as no more buffers are needed
	while (running && !needsMoreAudio()) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100us);
	}
	
	// When we get here, it means that buffers are needed again (either one wasn't enough, or we waited long enough),
	// but first return so we the AudioFacade can update the incoming sound data
}

void AudioEngine::addEmitter(uint32_t id, std::unique_ptr<AudioVoice> src)
{
	emitters.emplace_back(std::move(src));
	emitters.back()->setId(id);
	idToSource[id].push_back(emitters.back().get());
}

const std::vector<AudioVoice*>& AudioEngine::getSources(uint32_t id)
{
	auto src = idToSource.find(id);
	if (src != idToSource.end()) {
		return src->second;
	} else {
		return dummyIdSource;
	}
}

std::vector<uint32_t> AudioEngine::getFinishedSounds()
{
	return std::move(finishedSounds);
}

void AudioEngine::start(AudioSpec s, AudioOutputAPI& o)
{
	spec = s;
	out = &o;
	running = true;

	out->setAudioOutputInterface(*this);

	channels.resize(spec.numChannels);
	channels[0].pan = -1.0f;
	channels[1].pan = 1.0f;

	if (spec.sampleRate != 48000) {
		outResampler = std::make_unique<AudioResampler>(48000, spec.sampleRate, spec.numChannels, Debug::isDebug() ? 0.0f : 0.5f);
	}
}

void AudioEngine::resume()
{
	running = true;
	needsBuffer = false;
}

void AudioEngine::pause()
{
	running = false;
	needsBuffer = false;
}

void AudioEngine::generateBuffer()
{
	ProfilerEvent event(ProfilerEventType::AudioGenerateBuffer);
	Stopwatch timer;
	timer.start();
	
	const size_t samplesToRead = alignUp(spec.bufferSize * 48000 / spec.sampleRate, 16);
	const size_t packsToRead = samplesToRead / 16;
	const size_t numChannels = spec.numChannels;
	
	auto channelBuffersRef = pool->getBuffers(numChannels, samplesToRead);
	auto channelBuffers = channelBuffersRef.getBuffers();
	mixEmitters(samplesToRead, numChannels, channelBuffers);
	removeFinishedEmitters();

	// Interleave
	auto bufferRef = pool->getBuffer(samplesToRead * numChannels);
	auto buffer = bufferRef.getSpan().subspan(0, packsToRead * numChannels);
	const bool interleave = out->needsInterleavedSamples();
	if (interleave) {
		mixer->interleaveChannels(buffer, channelBuffers);
	} else {
		mixer->concatenateChannels(buffer, channelBuffers);
	}

	// Compress
	mixer->compressRange(buffer);

	// Resample to output sample rate, if necessary
	if (outResampler) {
		const auto resampledBuffer = pool->getBuffer(samplesToRead * numChannels * spec.sampleRate / 48000 + 16);
		const auto srcSpan = bufferRef.getSampleSpan().subspan(0, samplesToRead * numChannels);
		const auto dstSpan = resampledBuffer.getSampleSpan();
		auto result = interleave ? outResampler->resampleInterleaved(srcSpan, dstSpan) : outResampler->resampleNoninterleaved(srcSpan, dstSpan, numChannels);
		if (result.nRead != samplesToRead) {
			Logger::logError("Audio resampler failed to read all input sample data.");
		}
		queueAudioFloat(resampledBuffer.getSampleSpan().subspan(0, result.nWritten * numChannels));
	} else {
		queueAudioFloat(bufferRef.getSampleSpan());
	}

	timer.pause();
	lastTimeElapsed += timer.elapsedNanoseconds();
}

void AudioEngine::queueAudioFloat(gsl::span<const float> data)
{
	const size_t numSamples = data.size();

	// Float
	if (spec.format == AudioSampleFormat::Float) {
		queueAudioBytes(gsl::as_bytes(data));
	}

	// Int16
	else if (spec.format == AudioSampleFormat::Int16) {
		if (tmpShort.size() < numSamples) {
			tmpShort.resize(numSamples);
		}
		for (size_t i = 0; i < data.size(); ++i) {
			tmpShort[i] = static_cast<short>(data[i] * 32768.0f);
		}

		queueAudioBytes(gsl::as_bytes(gsl::span<short>(tmpShort)));
	}

	// Int32
	else if (spec.format == AudioSampleFormat::Int32) {
		if (tmpInt.size() < numSamples) {
			tmpInt.resize(numSamples);
		}
		for (size_t i = 0; i < data.size(); ++i) {
			tmpInt[i] = static_cast<int>(data[i] * 2147483648.0f);
		}

		queueAudioBytes(gsl::as_bytes(gsl::span<int>(tmpInt)));
	}
}

void AudioEngine::queueAudioBytes(gsl::span<const gsl::byte> data)
{
	if (audioOutputBuffer.canWrite(size_t(data.size()))) {
		audioOutputBuffer.write(data);
	} else {
		Logger::logError("Buffer overflow on audio output buffer.");
	}
	
	out->onAudioAvailable();
}

size_t AudioEngine::getAvailable()
{
	return audioOutputBuffer.availableToRead();
}

size_t AudioEngine::output(gsl::span<std::byte> dst, bool fill)
{
	size_t written = 0;
	if (!audioOutputBuffer.empty()) {
		written = std::min(size_t(dst.size()), audioOutputBuffer.availableToRead());
		audioOutputBuffer.read(dst.subspan(0, written));
	}

	const auto remaining = dst.subspan(written);
	if (!remaining.empty() && fill) {
		// :(
		Logger::logWarning("Insufficient audio data, padding with zeroes.");
		memset(remaining.data(), 0, size_t(remaining.size_bytes()));
		written = size_t(dst.size());
	}

	return written;
}

bool AudioEngine::needsMoreAudio()
{
	return out->needsMoreAudio();
}

Random& AudioEngine::getRNG()
{
	return rng;
}

AudioBufferPool& AudioEngine::getPool() const
{
	return *pool;
}

AudioVariableTable& AudioEngine::getVariableTable() const
{
	return *variableTable;
}

void AudioEngine::setMasterGain(float gain)
{
	masterGain = gain;
}

void AudioEngine::setGroupGain(const String& name, float gain)
{
	groupGains[getGroupId(name)] = gain;
}

void AudioEngine::mixEmitters(size_t numSamples, size_t nChannels, gsl::span<AudioBuffer*> buffers)
{
	// Clear buffers
	for (size_t i = 0; i < nChannels; ++i) {
		clearBuffer(buffers[i]->packs);
	}

	// Mix every emitter
	for (auto& e: emitters) {
		// Start playing if necessary
		if (!e->isPlaying() && !e->isDone() && e->isReady()) {
			e->start();
		}

		// Mix it in!
		if (e->isPlaying()) {
			e->update(channels, listener, masterGain * getGroupGain(e->getGroup()));
			e->mixTo(numSamples, buffers, *mixer, *pool);
		}
	}
}

void AudioEngine::removeFinishedEmitters()
{
	for (auto& e: emitters) {
		if (e->isDone()) {
			auto iter = idToSource.find(e->getId());
			if (iter != idToSource.end()) {
				auto& ems = iter->second;
				if (ems.size() > 1) {
					ems.erase(std::remove_if(ems.begin(), ems.end(), [] (const AudioVoice* e) { return e->isDone(); }), ems.end());
				} else {
					finishedSounds.push_back(iter->first);
					idToSource.erase(iter);
				}
			}
		}
	}
	emitters.erase(std::remove_if(emitters.begin(), emitters.end(), [&] (const std::unique_ptr<AudioVoice>& src) { return src->isDone(); }), emitters.end());
}

void AudioEngine::clearBuffer(gsl::span<AudioSamplePack> dst)
{
	for (auto& pack : dst) {
		for (size_t i = 0; i < AudioSamplePack::NumSamples; ++i) {
			pack.samples[i] = 0.0f;
		}
	}
}

int AudioEngine::getGroupId(const String& group)
{
	const auto iter = std::find(groupNames.begin(), groupNames.end(), group);
	if (iter != groupNames.end()) {
		return int(iter - groupNames.begin());
	} else {
		groupNames.push_back(group);
		groupGains.push_back(1.0f);
		return int(groupNames.size()) - 1;
	}
}

void AudioEngine::setVariable(const String& name, float value)
{
	variableTable->set(name, value);
}

int64_t AudioEngine::getLastTimeElapsed()
{
	return lastTimeElapsed.exchange(0);
}

float AudioEngine::getGroupGain(uint8_t id) const
{
	return groupGains[id];
}
