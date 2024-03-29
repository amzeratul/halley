#include "halley/audio/audio_facade.h"

#include "audio_emitter_handle_impl.h"
#include "audio_engine.h"
#include "audio_handle_impl.h"
#include "halley/support/console.h"
#include "halley/support/logger.h"
#include "halley/resources/resources.h"
#include "halley/audio/audio_event.h"
#include "halley/properties/game_properties.h"

using namespace Halley;

namespace {
	// The command queue is filled every update, and only drained when the audio thread runs.
	// Therefore its size must be > (logicFPS / audioFPS).
	// Because audio can run as low as 10 fps (for a >4096 sample buffer), and logic could conceivably be over 1000 fps,
	// this should be at least 100. Here I choose 256 as a safe number.
	constexpr size_t commandQueueSize = 256;

	// These, on the other hand, flow the other direction, so they can be much more relaxed
	constexpr size_t exceptionQueueSize = 16;
	constexpr size_t finishedSoundsQueueSize = 16;
}

AudioFacade::AudioFacade(AudioOutputAPI& o, SystemAPI& system)
	: output(o)
	, system(system)
	, running(false)
	, started(false)
	, commandQueue(commandQueueSize)
	, exceptions(exceptionQueueSize)
	, finishedSoundsQueue(finishedSoundsQueueSize)
	, ownAudioThread(o.needsAudioThread())
	, curEmitterId(1)
{
}

AudioFacade::~AudioFacade()
{
	AudioFacade::stopPlayback();
}

void AudioFacade::setResources(Resources& res)
{
	resources = &res;
}

void AudioFacade::init()
{
	std::cout << ConsoleColour(Console::GREEN) << "\nInitializing audio...\n" << ConsoleColour();
	std::cout << "Audio devices available:" << std::endl;
	int i = 0;
	for (auto& device: getAudioDevices()) {
		std::cout << "\t" << i++ << ": " << device->getName() << std::endl;
	}
}

void AudioFacade::deInit()
{
	stopPlayback();
}

Vector<std::unique_ptr<const AudioDevice>> AudioFacade::getAudioDevices()
{
	return output.getAudioDevices();
}

void AudioFacade::startPlayback(int deviceNumber)
{
	doStartPlayback(deviceNumber, true);
}

void AudioFacade::doStartPlayback(int deviceNumber, bool createEngine)
{
	if (createEngine && started) {
		stopPlayback();
	}

	auto devices = getAudioDevices();
	if (int(devices.size()) > deviceNumber) {
		if (createEngine) {
			engine = std::make_unique<AudioEngine>();
		}

		AudioSpec format;
		format.bufferSize = 512;
		format.format = AudioSampleFormat::Float;
		format.numChannels = 2;
		format.sampleRate = 48000;

		try {
			audioSpec = output.openAudioDevice(format, devices.at(deviceNumber).get(), [this]() { onNeedBuffer(); });
			started = true;
			lastDeviceNumber = deviceNumber;

			std::cout << "Audio Playback started.\n";
			std::cout << "\tDevice: " << devices.at(deviceNumber)->getName() << " [" << deviceNumber << "]\n";
			std::cout << "\tSample rate: " << audioSpec.sampleRate << "\n";
			std::cout << "\tChannels: " << audioSpec.numChannels << "\n";
			std::cout << "\tFormat: " << toString(audioSpec.format) << "\n";
			std::cout << "\tBuffer size: " << audioSpec.bufferSize << std::endl;

			resumePlayback();
		} catch (...) {
			// Unable to open audio device
		}
	}
}

void AudioFacade::stopPlayback()
{
	if (started) {
		pausePlayback();
		musicTracks.clear();
		engine.reset();
		output.closeAudioDevice();
		started = false;
	}
}

void AudioFacade::resumePlayback()
{
	if (started) {
		if (running) {
			pausePlayback();
		}

		engine->start(audioSpec, output, getAudioProperties());
		running = true;

		if (ownAudioThread) {
			audioThread = system.createThread("Audio", ThreadPriority::VeryHigh, [this]() { run(); });
		}

		output.startPlayback();
	}
}

void AudioFacade::pausePlayback()
{
	if (running) {
		{
			running = false;
			engine->pause();
		}
		if (ownAudioThread) {
			audioThread.join();
			audioThread = {};
		}
		output.stopPlayback();
	}
}

void AudioFacade::onSuspend()
{
	pausePlayback();
}

void AudioFacade::onResume()
{
	doStartPlayback(lastDeviceNumber, false);
}

int64_t AudioFacade::getLastTimeElapsed() const
{
	return running ? engine->getLastTimeElapsed() : 0;
}

std::optional<AudioSpec> AudioFacade::getAudioSpec() const
{
	return running ? audioSpec : std::optional<AudioSpec>();
}

void AudioFacade::setBufferSizeController(std::shared_ptr<IAudioBufferSizeController> controller)
{
	enqueue([=]() {
		engine->setBufferSizeController(controller);
	});
}


AudioEmitterHandle AudioFacade::createEmitter(AudioPosition position)
{
	const auto emitterId = curEmitterId++;

	enqueue([=]() {
		engine->createEmitter(emitterId, position, false);
	});

	return std::make_shared<AudioEmitterHandleImpl>(*this, emitterId, true);
}

AudioEmitterHandle AudioFacade::getGlobalEmitter()
{
	return std::make_shared<AudioEmitterHandleImpl>(*this, 0, false);
}

AudioHandle AudioFacade::postEvent(const String& name)
{
	return doPostEvent(name, 0);
}

AudioHandle AudioFacade::postEvent(const String& name, AudioEmitterHandle emitter)
{
	if (!emitter) {
		Logger::logError("Cannot post event \"" + name + "\" to invalid emitter.");
		return std::make_shared<AudioHandleImpl>(*this, curEventId++, 0);
	}
	return doPostEvent(name, emitter->getId());
}

AudioHandle AudioFacade::postEvent(const AudioEvent& event)
{
	return doPostEvent(event, 0);
}

AudioHandle AudioFacade::postEvent(const AudioEvent& event, AudioEmitterHandle emitter)
{
	if (!emitter) {
		Logger::logError("Cannot post event \"" + event.getAssetId() + "\" to invalid emitter.");
		return std::make_shared<AudioHandleImpl>(*this, curEventId++, 0);
	}
	return doPostEvent(event, emitter->getId());
}

AudioHandle AudioFacade::doPostEvent(const AudioEvent& event, AudioEmitterId emitterId)
{
	const auto id = curEventId++;

	enqueue([=, &event]() {
		engine->postEvent(id, event, emitterId);
	});
	playingSounds.push_back(id);

	return std::make_shared<AudioHandleImpl>(*this, id, emitterId);
}

AudioHandle AudioFacade::doPostEvent(const String& name, AudioEmitterId emitterId)
{
	if (resources->exists<AudioEvent>(name)) {
		const auto event = resources->get<AudioEvent>(name);
		return doPostEvent(*event, emitterId);
	} else {
		Logger::logError("Unknown audio event: \"" + name + "\"");
		const auto id = curEventId++;
		return std::make_shared<AudioHandleImpl>(*this, id, emitterId);
	}
}

AudioHandle AudioFacade::play(std::shared_ptr<const IAudioClip> clip, AudioEmitterHandle emitter, float volume, bool loop)
{
	uint32_t id = curEventId++;
	const auto emitterId = emitter ? emitter->getId() : 0;

	enqueue([=] () {
		engine->play(id, clip, emitterId, volume, loop);
	});
	playingSounds.push_back(id);

	return std::make_shared<AudioHandleImpl>(*this, id, emitterId);
}

AudioHandle AudioFacade::postEvent(const String& name, AudioPosition position)
{
	const auto id = curEventId++;
	const auto emitterId = curEmitterId++;

	if (resources->exists<AudioEvent>(name)) {
		const auto event = resources->get<AudioEvent>(name);
		enqueue([=]() {
			engine->createEmitter(emitterId, position, true);
			engine->postEvent(id, *event, emitterId);
		});
	} else {
		Logger::logError("Unknown audio event: \"" + name + "\"");
	}

	playingSounds.push_back(id);
	return std::make_shared<AudioHandleImpl>(*this, id, emitterId);
}

AudioHandle AudioFacade::play(std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop)
{
	uint32_t id = curEventId++;
	const auto emitterId = curEmitterId++;

	enqueue([=] () {
		engine->createEmitter(emitterId, position, true);
		engine->play(id, clip, emitterId, volume, loop);
	});
	playingSounds.push_back(id);
	return std::make_shared<AudioHandleImpl>(*this, id, emitterId);
}

AudioHandle AudioFacade::playMusic(const String& eventName, int track, float fadeInTime)
{
	stopMusic(track, 0.5f);
	auto handle = postEvent(eventName, AudioPosition::makeFixed());
	musicTracks[track] = handle;

	handle->play(AudioFade(fadeInTime, AudioFadeCurve::Linear));

	return handle;
}

AudioHandle AudioFacade::getMusic(int track)
{
	auto iter = musicTracks.find(track);
	if (iter != musicTracks.end()) {
		return iter->second;
	} else {
		return AudioHandle();
	}
}

void AudioFacade::stopMusic(int track, float fadeOutTime)
{
	auto iter = musicTracks.find(track);
	if (iter != musicTracks.end()) {
		stopMusic(iter->second, fadeOutTime);
		musicTracks.erase(iter);
	}
}

void AudioFacade::stopAllMusic(float fadeOutTime)
{
	for (auto& m: musicTracks) {
		stopMusic(m.second, fadeOutTime);
	}
	musicTracks.clear();
}

void AudioFacade::setMasterVolume(float volume)
{
	enqueue([=] () {
		engine->setMasterGain(volumeToGain(volume));
	});
}

void AudioFacade::setBusVolume(const String& busName, float volume)
{
	enqueue([=] () {
		engine->setBusGain(busName, volumeToGain(volume));
	});
}

void AudioFacade::setOutputChannels(Vector<AudioChannelData> audioChannelData)
{
	enqueue([=, audioChannelData = std::move(audioChannelData)] () mutable
	{
		engine->setOutputChannels(std::move(audioChannelData));
	});
}

void AudioFacade::stopMusic(AudioHandle& handle, float fadeOutTime)
{
	handle->stop(AudioFade(fadeOutTime, AudioFadeCurve::Linear));
}

void AudioFacade::onNeedBuffer()
{
	if (!ownAudioThread) {
		stepAudio();
	}
}

const AudioProperties& AudioFacade::getAudioProperties() const
{
	if (!resources->exists<GameProperties>("game_properties")) {
		static AudioProperties dummy;
		return dummy;
	}
	const auto properties = resources->get<GameProperties>("game_properties");
	return properties->getAudioProperties();
}

void AudioFacade::setListener(AudioListenerData listener)
{
	enqueue([=] () {
		engine->setListener(listener);
	});
}

void AudioFacade::onAudioException(std::exception& e)
{
	if (exceptions.canWrite(1)) {
		exceptions.writeOne(e.what());
	}
}

void AudioFacade::run()
{
	while (running) {
		stepAudio();
	}
}

void AudioFacade::stepAudio()
{
	try {
		{
			if (!running) {
				return;
			}
			if (finishedSoundsQueue.canWrite(1)) {
				auto finishedSounds = engine->getFinishedSounds();
				if (!finishedSounds.empty()) {
					finishedSoundsQueue.writeOne(std::move(finishedSounds));
				}
			}
		}

		while (commandQueue.canRead(1)) {
			commandQueue.read(gsl::span<Vector<std::function<void()>>>(&inbox, 1));
			for (auto& action: inbox) {
				action();
			}
			inbox.clear();
		}

		if (ownAudioThread) {
			engine->run();
		} else {
			engine->generateBuffer();
		}
	} catch (std::exception& e) {
		onAudioException(e);
	}
}

void AudioFacade::enqueue(std::function<void()> action)
{
	if (running) {
		outbox.push_back(std::move(action));
	}
}

void AudioFacade::pump()
{
	if (!exceptions.empty()) {
		String e;
		while (!exceptions.empty()) {
			e = exceptions.readOne();
			Logger::logError(e);
		}
		stopPlayback();
		throw Exception(e, HalleyExceptions::AudioEngine);
	}

	if (running) {
		if (!outbox.empty()) {
			if (commandQueue.canWrite(1)) {
				commandQueue.writeOne(std::move(outbox));
				outbox.clear();
			} else {
				Logger::logError("Out of space on audio command queue.");
			}
		}

		while (finishedSoundsQueue.canRead(1)) {
			auto finishedSounds = finishedSoundsQueue.readOne();
			playingSounds.erase(std::remove_if(playingSounds.begin(), playingSounds.end(), [&] (uint32_t id) -> bool
			{
				return std::find(finishedSounds.begin(), finishedSounds.end(), id) != finishedSounds.end();
			}), playingSounds.end());
		}
	}
}
