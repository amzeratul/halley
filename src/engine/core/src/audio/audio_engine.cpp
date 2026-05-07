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
	, lastTimeElapsed(0)
{
	rng.setSeed(Random::getGlobal().getRawInt());

	createEmitter(0, AudioPosition::makeFixed(), false);
	createRegion(0, "defaultRegion");
}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::createEmitter(AudioEmitterId id, AudioPosition position, bool temporary)
{
	emitters[id] = std::make_unique<AudioEmitter>(*this, id, std::move(position), temporary, id == 0 ? nullptr : emitters.at(0).get());
}

void AudioEngine::destroyEmitter(AudioEmitterId id)
{
	emitters.erase(id);
}

void AudioEngine::createRegion(AudioRegionId id, String name)
{
	regions[id] = std::make_unique<AudioRegion>(id, std::move(name));
}

void AudioEngine::destroyRegion(AudioRegionId id)
{
	regions.at(id)->markAsReadyToDestroy();
}

void AudioEngine::postEvent(AudioEventId id, const AudioEvent& event, AudioEmitterId emitterId)
{
	if (canLog(event.getAssetId())) {
		Logger::log(*eventLogging, "AudioEvent posted: " + event.getAssetId());
	}

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

void AudioEngine::play(AudioEventId id, std::shared_ptr<const IAudioClip> clip, AudioEmitterId emitterId, float gain, bool loop, AudioFade fade)
{
	const auto iter = emitters.find(emitterId);
	if (iter == emitters.end()) {
		finishedSounds.push_back(id);
		return;
	}

	auto voice = std::make_unique<AudioVoice>(*this, std::make_shared<AudioSourceClip>(*this, std::move(clip), loop, 1.0f, 0, 0, false), gain, 1.0f, 0.0f, 0, getBusId(""));
	voice->setIds(id);
	voice->play(fade);
	iter->second->addVoice(std::move(voice));
}

void AudioEngine::play(AudioEventId id, std::shared_ptr<const AudioObject> object, AudioEmitterId emitterId, float gain, AudioFade fade)
{
	const auto iter = emitters.find(emitterId);
	if (iter == emitters.end()) {
		finishedSounds.push_back(id);
		return;
	}

	if (auto voice = makeObjectVoice(*object, id, *iter->second, Range<float>(gain, gain))) {
		voice->play(fade);
		iter->second->addVoice(std::move(voice));
	}
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
	for (int i = 0; running && !needsMoreAudio(); ++i) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10us);

		if (i == 2000) {
			Logger::logError("Audio thread seems to be stalled", true);
		}
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

const AudioProperties& AudioEngine::getAudioProperties() const
{
	HalleyAssertDev(audioProperties != nullptr);
	return *audioProperties;
}

void AudioEngine::generateBuffer()
{
	ProfilerEvent event(ProfilerEventType::AudioGenerateBuffer, "", reinterpret_cast<uint64_t>(this));
	Stopwatch timer;
	timer.start();

	const size_t targetSamples = bufferSizeController ? bufferSizeController->getTargetSamples() : spec.bufferSize;
	const size_t samplesToRead = alignDown(targetSamples * 48000 / spec.sampleRate, static_cast<size_t>(16));
	const size_t numChannels = spec.numChannels;
	const float deltaTime = static_cast<float>(samplesToRead) / static_cast<float>(spec.sampleRate);

	updateRegions();
	computeBusGains(deltaTime);

	auto channelBuffersRef = pool->getBuffers(numChannels, samplesToRead);
	mixVoices(samplesToRead, numChannels, channelBuffersRef);
	removeFinishedVoices();
	updatePlayingObjectData(deltaTime);

	// Interleave
	auto bufferRef = pool->getBuffer(samplesToRead * numChannels);
	auto buffer = bufferRef.getSpan().subspan(0, samplesToRead * numChannels);
	const bool interleave = out->needsInterleavedSamples();
	if (interleave) {
		AudioMixer::interleaveChannels(buffer, channelBuffersRef.getBuffers());
	} else {
		AudioMixer::concatenateChannels(buffer, channelBuffersRef.getBuffers());
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

void AudioEngine::queueAudioBytes(gsl::span<const std::byte> data)
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
		Logger::logError("Insufficient audio data, padding with zeroes.", true);
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

void AudioEngine::mixVoices(size_t numSamples, size_t nChannels, AudioBuffersRef& buffers)
{
	// Clear buffers
	for (size_t i = 0; i < nChannels; ++i) {
		AudioMixer::zero(buffers[i].samples);
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
		if (auto iter = regions.find(listenerRegion.regionId); iter != regions.end()) {
			auto& region = *iter->second;

			if (!region.shouldDestroy()) {
				float gain = listenerRegion.presence;
				const float prevGain = region.getPrevGain();
				region.setPrevGain(gain);

				mixMainRegion(numSamples, nChannels, region, buffers, prevGain, gain);
			}
		}
	}

	// Clear voice buffers
	for (auto& e: emitters) {
		for (auto& v: e.second->getVoices()) {
			v->clearBuffers();
		}
	}
}

void AudioEngine::mixMainRegion(size_t numSamples, size_t nChannels, AudioRegion& region, AudioBuffersRef& outputBuffers, float prevGain, float gain)
{
	mixRegion(region, outputBuffers, prevGain, gain);

	for (auto& neighbour: region.getNeighbours()) {
		if (auto iter = regions.find(neighbour.props.id); iter != regions.end()) {
			const auto& otherRegion = *iter->second;
			const float gain0 = prevGain * neighbour.props.attenuation;
			const float gain1 = gain * neighbour.props.attenuation;

			if (neighbour.props.lowPassHz) {
				auto regionBuffer = pool->getBuffers(nChannels, numSamples, true);
				mixRegion(otherRegion, regionBuffer, 1, 1);

				neighbour.filter.processSamples(regionBuffer);

				AudioMixer::mixAudio(regionBuffer.getSampleSpans(), outputBuffers.getSampleSpans(), gain0, gain1);
			} else {
				mixRegion(otherRegion, outputBuffers, gain0, gain1);
			}
		} else {
			Logger::logError("Audio Region \"" + region.getName() + "\" has unknown neighbour \"" + neighbour.name + "\"", true);
		}
	}
}

void AudioEngine::mixRegion(const AudioRegion& region, AudioBuffersRef& buffers, float prevGain, float gain)
{
	for (auto& e: emitters) {
		const auto regionId = e.second->getRegion();
		if (regionId == region.getId()) {
			for (auto& v: e.second->getVoices()) {
				if (v->isPlaying()) {
					v->mixTo(buffers.getBuffers(), prevGain, gain);
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
		return -1;
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
				iter->second->incRefCount(*this);
			} else {
				Logger::logWarning("Unknown region referenced by audio emitter: " + toString(static_cast<int>(regionId)));
			}
		}
	}

	for (auto& listenerRegion: listener.regions) {
		const auto regionId = listenerRegion.regionId;
		if (auto iter = regions.find(regionId); iter != regions.end()) {
			iter->second->incRefCount(*this);
		} else {
			Logger::logWarning("Unknown region referenced by audio emitter: " + toString(static_cast<int>(regionId)));
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
	HashMap<String, Vector<BusGain>> prevGains;
	prevGains.reserve(buses.size());
	for (auto& bus: buses) {
		prevGains[bus.name] = std::move(bus.gains);
	}

	buses.clear();
	buses.reserve(countBuses(audioProperties->getBuses()));

	for (const auto& bus: audioProperties->getBuses()) {
		loadBus(bus, OptionalLite<uint8_t>{});
	}

	for (auto& bus: buses) {
		if (prevGains.contains(bus.name)) {
			bus.gains = std::move(prevGains.at(bus.name));
		}
	}

	computeBusGains(0.0f);
}

uint8_t AudioEngine::loadBus(const AudioBusProperties& bus, OptionalLite<uint8_t> parent)
{
	const auto id = static_cast<uint8_t>(buses.size());
	buses.push_back(BusData{ bus.getId(), 1.0f, parent, {}, {} });
	for (const auto& child: bus.getChildren()) {
		buses[id].children.push_back(loadBus(child, id));
	}

	return id;
}

void AudioEngine::computeBusGains(float deltaTime)
{
	for (auto& bus: buses) {
		const float base = bus.parent ? buses.at(bus.parent.value()).compositeGain : 1.0f;
		bus.compositeGain = base;
		for (auto& gain: bus.gains) {
			gain.fader.update(deltaTime);
			bus.compositeGain *= gain.fader.getCurrentValue();
		}
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

bool AudioEngine::setBusGain(const String& name, float gain, AudioFade fade, const String& volumeName)
{
	const auto busId = getBusId(name);
	if (busId < 0) {
		return false;
	}

	auto& busGain = buses[busId].getGain(volumeName);
	busGain.fader.startFade(busGain.fader.getCurrentValue(), gain, fade);

	return true;
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

std::unique_ptr<AudioVoice> AudioEngine::makeObjectVoice(const AudioObject& object, AudioEventId uniqueId, AudioEmitter& emitter, Range<float> playGain, Range<float> playPitch, uint32_t delaySamples)
{
	// Prune if out of range
	if (object.getPruneDistant()) {
		if (emitter.getPosition().getAttenuation(listener, object.getAttenuationOverride()) < 0.000001f) {
			if (canLog(object.getAssetId())) {
				Logger::log(*eventLogging, "- Prunned due to distance to listener: " + object.getAssetId());
			}
			return {};
		}
	}

	auto [playingData, playStatus] = tryToStartVoiceForObject(object, emitter);
	if (playStatus != VoiceAllocationResult::Playing) {
		if (canLog(object.getAssetId())) {
			if (playStatus == VoiceAllocationResult::InstanceLimited) {
				Logger::log(*eventLogging, "- Instance limited: " + object.getAssetId());
			} else if (playStatus == VoiceAllocationResult::CooldownLimited) {
				Logger::log(*eventLogging, "- Cooldown limited: " + object.getAssetId());
			}
		}
		return {};
	}

	const auto gainRange = object.getGain() * playGain;
	const auto pitchRange = object.getPitch() * playPitch;
	const float gain = getRNG().getFloat(gainRange);
	const float pitch = clamp(getRNG().getFloat(pitchRange), 0.1f, 4.0f);
	const float dopplerScale = object.getDopplerScale();

	auto source = object.makeSource(*this, emitter);
	auto voice = std::make_unique<AudioVoice>(*this, std::move(source), gain, pitch, dopplerScale, delaySamples, getBusId(object.getBus()));

	voice->setIds(uniqueId, object.getAudioObjectId());
	voice->setMixingProperties(object.getMixingProperties());

	if (playingData) {
		playingData->playingVoices += voice.get();
	}

	return voice;
}

void AudioEngine::onVoiceFinished(const AudioVoice& voice)
{
	if (auto* data = tryGetMutablePlayingObjectData(voice.getAudioObjectId())) {
		std_ex::erase(data->playingVoices, &voice);
	}
}

const String& AudioEngine::getSwitchDefault(const String& switchId) const
{
	if (const auto* switchProps = audioProperties->tryGetSwitch(switchId)) {
		return switchProps->getDefaultValue();
	}
	return String::emptyString();
}

void AudioEngine::setEventLogging(std::optional<LoggerLevel> level, std::optional<String> prefix)
{
	eventLogging = level;
	eventLogPrefix = std::move(prefix);
}

void AudioEngine::resetObjectLimits()
{
	playingObjectData.clear();
}

void AudioEngine::resetBuses()
{
	for (auto& bus: buses) {
		bus.gains.clear();
	}
}

AudioEngine::BusGain::BusGain(String id)
	: id (std::move(id))
{
	fader.stopAndSetValue(1.0f);
}

AudioEngine::BusGain& AudioEngine::BusData::getGain(const String& name)
{
	for (auto& gain: gains) {
		if (gain.id == name) {
			return gain;
		}
	}
	gains.push_back(BusGain(name));
	return gains.back();
}

AudioDebugData AudioEngine::generateDebugData() const
{
	AudioDebugData result;

	result.emitters.reserve(emitters.size());
	for (const auto& [emitterId, emitter]: emitters) {
		result.emitters.emplace_back(emitter->getDebugData());
	}

	result.listener = listener;

	return result;
}

bool AudioEngine::canLog(const String& assetId) const
{
	return eventLogging && (!eventLogPrefix || assetId.startsWith(*eventLogPrefix));
}

AudioEngine::PlayingObjectData& AudioEngine::getMutablePlayingObjectData(AudioObjectId id)
{
	for (auto& data: playingObjectData) {
		if (data.id == id) {
			return data;
		}
	}
	return playingObjectData.emplace_back(PlayingObjectData{ id, 0, {} });
}

AudioEngine::PlayingObjectData* AudioEngine::tryGetMutablePlayingObjectData(AudioObjectId id)
{
	for (auto& data: playingObjectData) {
		if (data.id == id) {
			return &data;
		}
	}
	return nullptr;
}

void AudioEngine::updatePlayingObjectData(float deltaTime)
{
	for (auto& data: playingObjectData) {
		data.cooldown -= deltaTime;
	}

	std_ex::erase_if(playingObjectData, [] (const PlayingObjectData& entry)
	{
		return entry.playingVoices.empty() && entry.cooldown <= 0;
	});
}

std::pair<AudioEngine::PlayingObjectData*, AudioEngine::VoiceAllocationResult> AudioEngine::tryToStartVoiceForObject(const AudioObject& object, const AudioEmitter& emitter)
{
	if (!object.getMaxInstances() && !object.getCooldown()) {
		return { nullptr, VoiceAllocationResult::Playing };
	}

	auto* playingData = &getMutablePlayingObjectData(object.getAudioObjectId());

	// Sound is on cooldown
	if (playingData->cooldown > 0) {
		return { nullptr, VoiceAllocationResult::CooldownLimited };
	}

	// Max instances exceeded
	if (object.getMaxInstances() && static_cast<int>(playingData->playingVoices.size()) >= object.getMaxInstances()) {
		const bool canPlay = onVoiceLimitReached(*playingData, object.getInstanceLimitType(), emitter);
		if (!canPlay) {
			return { nullptr, VoiceAllocationResult::InstanceLimited };
		}
	}

	// Update cooldown
	if (object.getCooldown()) {
		playingData->cooldown = *object.getCooldown();
	}

	return { playingData, VoiceAllocationResult::Playing };
}

bool AudioEngine::onVoiceLimitReached(PlayingObjectData& data, AudioObjectInstanceLimitType limitType, const AudioEmitter& emitter)
{
	if (limitType == AudioObjectInstanceLimitType::DontPlay) {
		return false;
	}

	if (limitType == AudioObjectInstanceLimitType::StopOldest) {
		data.stop(0);
		return true;
	}

	if (limitType == AudioObjectInstanceLimitType::StopFarthest) {
		// Populate with my data
		int bestIdx = -1;
		float bestDistance = emitter.getPosition().getDistance(listener);

		for (int i = 0; i < static_cast<int>(data.playingVoices.size()); ++i) {
			const float dist = data.playingVoices[i]->getDistance();
			if (dist > bestDistance) {
				bestIdx = i;
			}
		}

		if (bestIdx == -1) {
			// I'm the farthest
			return false;
		} else {
			data.stop(bestIdx);
			return true;
		}
	}

	return true;
}

void AudioEngine::PlayingObjectData::stop(size_t idx)
{
	if (playingVoices.size() > idx) {
		playingVoices[idx]->stop({});
		playingVoices.erase(playingVoices.begin() + idx);
	}
}
