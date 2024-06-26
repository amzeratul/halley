#include "audio_engine.h"
#include "audio_mixer.h"
#include <thread>
#include <chrono>
#include "audio_sources/audio_source_clip.h"
#include "audio_filter_resample.h"
#include "halley/support/debug.h"
#include "halley/resources/resources.h"
#include "halley/audio/audio_event.h"
#include "halley/support/logger.h"
#include "halley/api/audio_api.h"
#include "halley/audio/audio_object.h"
#include "halley/properties/audio_properties.h"
#include "halley/support/profiler.h"
#include "halley/time/stopwatch.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

AudioEngine::AudioEngine()
	: pool(std::make_unique<AudioBufferPool>())
	, audioOutputBuffer(4096 * 8)
	, running(true)
	, needsBuffer(true)
{
	rng.setSeed(Random::getGlobal().getRawInt());

	createEmitter(0, AudioPosition::makeFixed(), false);
	createRegion(0);
}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::createEmitter(AudioEmitterId id, AudioPosition position, bool temporary)
{
	emitters[id] = std::make_unique<AudioEmitter>(id, std::move(position), temporary, id == 0 ? nullptr : emitters.at(0).get());
}

void AudioEngine::destroyEmitter(AudioEmitterId id)
{
	emitters.erase(id);
}

void AudioEngine::createRegion(AudioRegionId id)
{
	regions[id] = std::make_unique<AudioRegion>(id);
}

void AudioEngine::destroyRegion(AudioRegionId id)
{
	regions.at(id)->markAsReadyToDestroy();
}

void AudioEngine::postEvent(AudioEventId id, const AudioEvent& event, AudioEmitterId emitterId)
{
	const auto iter = emitters.find(emitterId);
	if (iter == emitters.end()) {
		finishedSounds.push_back(id);
		return;
	}

	const size_t nVoices = event.run(*this, id, *emitters.at(0), *iter->second);
	if (nVoices == 0) {
		finishedSounds.push_back(id);
	}
}

void AudioEngine::play(AudioEventId id, std::shared_ptr<const IAudioClip> clip, AudioEmitterId emitterId, float volume, bool loop)
{
	const auto iter = emitters.find(emitterId);
	if (iter == emitters.end()) {
		finishedSounds.push_back(id);
		return;
	}

	auto voice = std::make_unique<AudioVoice>(*this, std::make_shared<AudioSourceClip>(*this, std::move(clip), loop, 1.0f, 0, 0, false), volume, 1.0f, 0.0f, 0, getBusId(""));
	voice->setIds(id);
	iter->second->addVoice(std::move(voice));
}

void AudioEngine::play(AudioEventId id, std::shared_ptr<const AudioObject> object, AudioEmitterId emitterId, float volume)
{
	const auto iter = emitters.find(emitterId);
	if (iter == emitters.end()) {
		finishedSounds.push_back(id);
		return;
	}

	auto pitch = rng.getFloat(object->getPitch());
	auto voice = std::make_unique<AudioVoice>(*this, object->makeSource(*this, *iter->second), volume, pitch, 0.0f, 0, getBusId(object->getBus()));
	voice->setIds(id);
	iter->second->addVoice(std::move(voice));
}

void AudioEngine::setListener(AudioListenerData l)
{
	listener = std::move(l);
	if (!std_ex::contains_if(listener.regions, [] (const AudioListenerRegionData& rd) { return rd.regionId == 0; })) {
		listener.regions.emplace_back(); // Ensure the default region is added
	}
}

void AudioEngine::setOutputChannels(Vector<AudioChannelData> channelData)
{
	if (channels.size() == channelData.size()) {
		channels = std::move(channelData);
	}
}

void AudioEngine::run()
{
	//const size_t bufSize = spec.numChannels * sizeof(AudioSample) * spec.bufferSize;

	// Generate one buffer
	if (running && needsMoreAudio()) {
		generateBuffer();
	}

	// OK, we've supplied it with enough buffers; if that was enough, then, sleep as long as no more buffers are needed
	while (running && !needsMoreAudio()) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10us);
	}
	
	// When we get here, it means that buffers are needed again (either one wasn't enough, or we waited long enough),
	// but first return so we the AudioFacade can update the incoming sound data
}

void AudioEngine::forVoices(AudioObjectId audioObjectId, VoiceCallback callback)
{
	for (auto& e: emitters) {
		for (auto& v: e.second->getVoices()) {
			if (v->getAudioObjectId() == audioObjectId) {
				callback(*v);
			}
		}
	}
}

void AudioEngine::forVoicesOnBus(int busId, VoiceCallback callback)
{
	for (auto& e: emitters) {
		for (auto& v: e.second->getVoices()) {
			if (v->getBus() == busId) {
				callback(*v);
			}
		}
	}
}

AudioEmitter* AudioEngine::getEmitter(AudioEmitterId id)
{
	const auto iter = emitters.find(id);
	if (iter == emitters.end()) {
		return nullptr;
	}
	return iter->second.get();
}

AudioRegion* AudioEngine::getRegion(AudioRegionId id)
{
	const auto iter = regions.find(id);
	if (iter == regions.end()) {
		return nullptr;
	}
	return iter->second.get();
}

Vector<AudioEventId> AudioEngine::getFinishedSounds()
{
	return std::move(finishedSounds);
}

void AudioEngine::start(AudioSpec s, AudioOutputAPI& o, const AudioProperties& audioProperties)
{
	spec = s;
	out = &o;
	this->audioProperties = &audioProperties;
	running = true;

	out->setAudioOutputInterface(*this);

	channels.resize(spec.numChannels);
	channels[0].pan = -1.0f;
	channels[1].pan = 1.0f;

	if (spec.sampleRate != 48000) {
		outResampler = std::make_unique<AudioResampler>(48000.0f, static_cast<float>(spec.sampleRate), spec.numChannels, Debug::isDebug() ? 0.0f : 0.5f);
	}

	loadBuses();
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

	updateRegions();
	updateBusGains();

	const size_t targetSamples = bufferSizeController ? bufferSizeController->getTargetSamples() : spec.bufferSize;
	const size_t samplesToRead = alignDown(targetSamples * 48000 / spec.sampleRate, static_cast<size_t>(16));
	const size_t numChannels = spec.numChannels;
	
	auto channelBuffersRef = pool->getBuffers(numChannels, samplesToRead);
	auto channelBuffers = channelBuffersRef.getBuffers();
	mixVoices(samplesToRead, numChannels, channelBuffers);
	removeFinishedVoices();

	// Interleave
	auto bufferRef = pool->getBuffer(samplesToRead * numChannels);
	auto buffer = bufferRef.getSpan().subspan(0, samplesToRead * numChannels);
	const bool interleave = out->needsInterleavedSamples();
	if (interleave) {
		AudioMixer::interleaveChannels(buffer, channelBuffers);
	} else {
		AudioMixer::concatenateChannels(buffer, channelBuffers);
	}

	// Compress
	AudioMixer::compressRange(buffer);

	// Resample to output sample rate, if necessary
	const auto srcSpan = bufferRef.getSpan().subspan(0, samplesToRead * numChannels);
	if (outResampler) {
		const auto resampledBuffer = pool->getBuffer(samplesToRead * numChannels * spec.sampleRate / 48000 + 16);
		const auto dstSpan = resampledBuffer.getSpan();
		auto result = interleave ? outResampler->resampleInterleaved(srcSpan, dstSpan) : outResampler->resampleNonInterleaved(srcSpan, dstSpan, numChannels);
		if (result.nRead != samplesToRead) {
			Logger::logError("Audio resampler failed to read all input sample data.");
		}
		queueAudioFloat(resampledBuffer.getSpan().subspan(0, result.nWritten * numChannels));
	} else {
		queueAudioFloat(srcSpan);
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

void AudioEngine::setMasterGain(float gain)
{
	masterGain = gain;
}

void AudioEngine::mixVoices(size_t numSamples, size_t nChannels, gsl::span<AudioBuffer*> buffers)
{
	// Clear buffers
	for (size_t i = 0; i < nChannels; ++i) {
		AudioMixer::zero(buffers[i]->samples);
	}

	// Update every emitter
	for (auto& e: emitters) {
		for (auto& v: e.second->getVoices()) {
			// Start playing if necessary
			if (!v->isPlaying() && !v->isDone() && v->isReady()) {
				v->start();
			}
			// Render
			if (v->isPlaying()) {
				v->update(channels, e.second->getPosition(), listener, masterGain * getCompositeBusGain(v->getBus()));
				v->render(numSamples, *pool);
			}
		}
	}

	// Mix every region
	for (auto& listenerRegion: listener.regions) {
		auto& region = *regions.at(listenerRegion.regionId);

		float gain = listenerRegion.presence;
		const float prevGain = region.getPrevGain();
		region.setPrevGain(gain);

		mixMainRegion(region, buffers, prevGain, gain);
	}

	// Clear voice buffers
	for (auto& e: emitters) {
		for (auto& v: e.second->getVoices()) {
			v->clearBuffers();
		}
	}
}

void AudioEngine::mixMainRegion(const AudioRegion& region, gsl::span<AudioBuffer*> buffers, float prevGain, float gain)
{
	mixRegion(region, buffers, prevGain, gain);

	for (const auto& neighbour: region.getNeighbours()) {
		if (auto iter = regions.find(neighbour.id); iter != regions.end()) {
			const auto& otherRegion = *iter->second;
			const float gain0 = prevGain * neighbour.attenuation;
			const float gain1 = gain * neighbour.attenuation;

			if (neighbour.lowPassHz) {
				// TODO: lowpass
				mixRegion(otherRegion, buffers, gain0, gain1);
			} else {
				mixRegion(otherRegion, buffers, gain0, gain1);
			}
		} else {
			Logger::logError("Audio Region " + toString(int(region.getId())) + " has unknown neighbour " + toString(int(neighbour.id)), true);
		}
	}
}

void AudioEngine::mixRegion(const AudioRegion& region, gsl::span<AudioBuffer*> buffers, float prevGain, float gain)
{
	for (auto& e: emitters) {
		const auto regionId = e.second->getRegion();
		if (regionId == region.getId()) {
			for (auto& v: e.second->getVoices()) {
				if (v->isPlaying()) {
					v->mixTo(buffers, prevGain, gain);
				}
			}
		}
	}
}

void AudioEngine::removeFinishedVoices()
{
	for (auto& e: emitters) {
		e.second->removeFinishedVoices(finishedSounds);
	}
	std_ex::erase_if_value(emitters, [&] (const std::unique_ptr<AudioEmitter>& src) { return src->shouldBeRemoved(); });
}

int AudioEngine::getBusId(const String& busName)
{
	const auto iter = std::find_if(buses.begin(), buses.end(), [&] (const BusData& b) { return b.name == busName; });
	if (iter != buses.end()) {
		return int(iter - buses.begin());
	} else {
		if (!busName.isEmpty()) {
			Logger::logError("Unknown audio bus: " + busName);
		}
		return 0;
	}
}

void AudioEngine::getBusIds(const String& busName, Vector<int>& busIds)
{
	busIds.clear();

	const auto iter = std::find_if(buses.begin(), buses.end(), [&] (const BusData& b) { return b.name == busName; });
	if (iter != buses.end()) {
		busIds.push_back(static_cast<int>(iter - buses.begin()));
		collectBusChildren(busIds, *iter);
	} else {
		if (busName.isEmpty()) {
			busIds.push_back(0);
		} else {
			Logger::logError("Unknown audio bus: " + busName);
		}
	}
}

void AudioEngine::collectBusChildren(Vector<int>& dst, const BusData& bus) const
{
	for (const auto child: bus.children) {
		dst.push_back(child);
		collectBusChildren(dst, buses[child]);
	}
}

void AudioEngine::updateRegions()
{
	for (auto& region: regions) {
		region.second->clearRefCount();
	}
	for (auto& emitter: emitters) {
		const auto regionId = emitter.second->getRegion();
		if (regionId != 0) {
			if (auto iter = regions.find(regionId); iter != regions.end()) {
				iter->second->incRefCount();
			} else {
				Logger::logWarning("Unknown region referenced by audio emitter: " + toString(int(regionId)));
			}
		}
	}

	std_ex::erase_if_value(regions, [&] (std::unique_ptr<AudioRegion>& region)
	{
		return region->shouldDestroy();
	});
}

namespace {
	size_t countBuses(gsl::span<const AudioBusProperties> buses)
	{
		size_t n = buses.size();
		for (const auto& bus: buses) {
			n += countBuses(bus.getChildren());
		}
		return n;
	}
}

void AudioEngine::loadBuses()
{
	buses.clear();
	buses.reserve(countBuses(audioProperties->getBuses()));

	for (const auto& bus: audioProperties->getBuses()) {
		loadBus(bus, OptionalLite<uint8_t>{});
	}
}

uint8_t AudioEngine::loadBus(const AudioBusProperties& bus, OptionalLite<uint8_t> parent)
{
	const auto id = static_cast<uint8_t>(buses.size());
	buses.push_back(BusData{ bus.getId(), 1.0f, 1.0f, parent });
	for (const auto& child: bus.getChildren()) {
		buses[id].children.push_back(loadBus(child, id));
	}

	return id;
}

void AudioEngine::updateBusGains()
{
	for (auto& bus: buses) {
		const float base = bus.parent ? buses.at(bus.parent.value()).compositeGain : 1.0f;
		bus.compositeGain = bus.gain * base;
	}
}

int64_t AudioEngine::getLastTimeElapsed()
{
	return lastTimeElapsed.exchange(0);
}

void AudioEngine::setBufferSizeController(std::shared_ptr<IAudioBufferSizeController> controller)
{
	bufferSizeController = std::move(controller);
}

void AudioEngine::setBusGain(const String& name, float gain)
{
	buses[getBusId(name)].gain = gain;
}

float AudioEngine::getCompositeBusGain(uint8_t id) const
{
	if (id >= buses.size()) {
		return 1.0f;
	}
	return buses.at(id).compositeGain;
}

void AudioEngine::setGenerateDebugData(bool enabled)
{
	debugDataEnabled = enabled;
}

std::optional<AudioDebugData> AudioEngine::getDebugData() const
{
	if (debugDataEnabled) {
		return generateDebugData();
	} else {
		return std::nullopt;
	}
}

AudioDebugData AudioEngine::generateDebugData() const
{
	AudioDebugData result;

	result.emitters.reserve(emitters.size());
	for (const auto& [emitterId, emitter]: emitters) {
		result.emitters.emplace_back(emitter->getDebugData());
	}

	return result;
}
