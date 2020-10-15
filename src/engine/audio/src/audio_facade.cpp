#include "audio_facade.h"
#include "audio_engine.h"
#include "audio_handle_impl.h"
#include "behaviours/audio_voice_behaviour.h"
#include "halley/support/console.h"
#include "halley/support/logger.h"
#include "halley/core/resources/resources.h"
#include "audio_event.h"
#include "behaviours/audio_voice_fade_behaviour.h"

using namespace Halley;

AudioFacade::AudioFacade(AudioOutputAPI& o, SystemAPI& system)
	: output(o)
	, system(system)
	, running(false)
	, started(false)
	, commandQueue(1024)
	, exceptions(16)
	, finishedSoundsQueue(4)
	, ownAudioThread(o.needsAudioThread())
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

		engine->start(audioSpec, output);
		running = true;

		if (ownAudioThread) {
			audioThread = system.createThread("Audio", ThreadPriority::High, [this]() { run(); });
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
	return engine->getLastTimeElapsed();
}

std::optional<AudioSpec> AudioFacade::getAudioSpec() const
{
	return running ? audioSpec : std::optional<AudioSpec>();
}

AudioHandle AudioFacade::postEvent(const String& name, AudioPosition position)
{
	uint32_t id = uniqueId++;

	if (resources->exists<AudioEvent>(name)) {
		const auto event = resources->get<AudioEvent>(name);
		enqueue([=]() {
			engine->postEvent(id, *event, position);
		});
	} else {
		Logger::logWarning("Unknown audio event: \"" + name + "\"");
	}

	playingSounds.push_back(id);
	return std::make_shared<AudioHandleImpl>(*this, id);
}

AudioHandle AudioFacade::play(std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop)
{
	uint32_t id = uniqueId++;
	enqueue([=] () {
		engine->play(id, clip, position, volume, loop);
	});
	playingSounds.push_back(id);
	return std::make_shared<AudioHandleImpl>(*this, id);
}

AudioHandle AudioFacade::playMusic(const String& eventName, int track, float fadeInTime)
{
	bool hasFade = fadeInTime > 0.0001f;
	
	stopMusic(track, 0.5f);
	auto handle = postEvent(eventName, AudioPosition::makeFixed());
	musicTracks[track] = handle;

	if (hasFade) {
		handle->setGain(0.0f);
		handle->addBehaviour(std::make_unique<AudioVoiceFadeBehaviour>(fadeInTime, 0.0f, 1.0f, false));
	}

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

void AudioFacade::setGroupVolume(const String& groupName, float volume)
{
	enqueue([=] () {
		engine->setGroupGain(groupName, volumeToGain(volume));
	});
}

void AudioFacade::setOutputChannels(std::vector<AudioChannelData> audioChannelData)
{
	enqueue([=, audioChannelData = std::move(audioChannelData)] () mutable
	{
		engine->setOutputChannels(std::move(audioChannelData));
	});
}

void AudioFacade::stopMusic(AudioHandle& handle, float fadeOutTime)
{
	if (fadeOutTime > 0.001f) {
		handle->addBehaviour(std::make_unique<AudioVoiceFadeBehaviour>(fadeOutTime, 1.0f, 0.0f, true));
	} else {
		handle->stop();
	}
}

void AudioFacade::onNeedBuffer()
{
	if (!ownAudioThread) {
		stepAudio();
	}
}

void AudioFacade::setListener(AudioListenerData listener)
{
	enqueue([=] () {
		engine->setListener(listener);
	});
}

void AudioFacade::setGlobalVariable(const String& variable, float value)
{
	String nameCopy = variable;
	enqueue([this, nameCopy = std::move(nameCopy), value]()
	{
		engine->setVariable(nameCopy, value);
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

		const size_t nToRead = commandQueue.availableToRead();
		inbox.resize(nToRead);
		commandQueue.read(gsl::span<std::function<void()>>(inbox.data(), nToRead));
		for (auto& action : inbox) {
			action();
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
		if (commandQueue.canWrite(1)) {
			commandQueue.writeOne(std::move(action));
		} else {
			Logger::logError("Out of space on audio command queue.");
		}
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
		while (finishedSoundsQueue.canRead(1)) {
			auto finishedSounds = finishedSoundsQueue.readOne();
			playingSounds.erase(std::remove_if(playingSounds.begin(), playingSounds.end(), [&] (uint32_t id) -> bool
			{
				return std::find(finishedSounds.begin(), finishedSounds.end(), id) != finishedSounds.end();
			}), playingSounds.end());
		}
	}
}
