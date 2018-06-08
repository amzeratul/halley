#include "audio_facade.h"
#include "audio_engine.h"
#include "audio_handle_impl.h"
#include "audio_emitter_behaviour.h"
#include "halley/support/console.h"
#include "halley/support/logger.h"
#include "halley/core/resources/resources.h"
#include "audio_event.h"

using namespace Halley;

AudioFacade::AudioFacade(AudioOutputAPI& o, SystemAPI& system)
	: output(o)
	, system(system)
	, running(false)
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
	if (started) {
		stopPlayback();
	}

	auto devices = getAudioDevices();
	if (int(devices.size()) > deviceNumber) {
		engine = std::make_unique<AudioEngine>();

		AudioSpec format;
		format.bufferSize = 1024;
		format.format = AudioSampleFormat::Float;
		format.numChannels = 2;
		format.sampleRate = 48000;
		
		audioSpec = output.openAudioDevice(format, devices.at(deviceNumber).get(), [this]() { onNeedBuffer(); });
		started = true;

		std::cout << "Audio Playback started.\n";
		std::cout << "\tDevice: " << devices.at(deviceNumber)->getName() << " [" << deviceNumber << "]\n";
		std::cout << "\tSample rate: " << audioSpec.sampleRate << "\n";
		std::cout << "\tChannels: " << audioSpec.numChannels << "\n";
		std::cout << "\tFormat: " << toString(audioSpec.format) << "\n";
		std::cout << "\tBuffer size: " << audioSpec.bufferSize << std::endl;

		resumePlayback();
	}
}

void AudioFacade::stopPlayback()
{
	if (started) {
		pausePlayback();
		musicTracks.clear();
		engine.reset();
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
			audioThread = system.createThread("Audio", [this]() { run(); });
		}

		output.startPlayback();
	}
}

void AudioFacade::pausePlayback()
{
	if (running) {
		{
			std::unique_lock<std::mutex> lock(audioMutex);
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

AudioHandle AudioFacade::postEvent(const String& name, AudioPosition position)
{
	auto event = resources->get<AudioEvent>(name);
	event->loadDependencies(*resources);

	size_t id = uniqueId++;
	enqueue([=] () {
		engine->postEvent(id, event, position);
	});
	return std::make_shared<AudioHandleImpl>(*this, id);
}

AudioHandle AudioFacade::playMusic(const String& eventName, int track)
{
	float fadeInTime = 0;
	bool hasFade = fadeInTime > 0.0001f;
	
	stopMusic(track, 0.5f);
	auto handle = postEvent(eventName, AudioPosition::makeFixed());
	musicTracks[track] = handle;

	if (hasFade) {
		handle->setGain(0.0f);
		handle->setBehaviour(std::make_unique<AudioEmitterFadeBehaviour>(fadeInTime, 1.0f, false));
	}

	return handle;
}

AudioHandle AudioFacade::play(std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop)
{
	size_t id = uniqueId++;
	enqueue([=] () {
		engine->play(id, clip, position, volume, loop);
	});
	return std::make_shared<AudioHandleImpl>(*this, id);
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

void AudioFacade::stopMusic(AudioHandle& handle, float fadeOutTime)
{
	if (fadeOutTime > 0.001f) {
		handle->setBehaviour(std::make_unique<AudioEmitterFadeBehaviour>(fadeOutTime, 0.0f, true));
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

void AudioFacade::onAudioException(std::exception& e)
{
	std::unique_lock<std::mutex> lock(exceptionMutex);
	exceptions.push_back(e.what());
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
		std::vector<std::function<void()>> toDo;
		{
			std::unique_lock<std::mutex> lock(audioMutex);
			if (!running) {
				return;
			}
			toDo = std::move(inbox);
			inbox.clear();
			playingSoundsNext = engine->getPlayingSounds();
		}

		for (auto& action : toDo) {
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
		outbox.push_back(action);
	}
}

void AudioFacade::pump()
{
	{
		std::unique_lock<std::mutex> lock(exceptionMutex);
		if (!exceptions.empty()) {
			for (size_t i = 0; i + 1 < exceptions.size(); ++i) {
				Logger::logError(exceptions[i]);
			}
			const auto e = exceptions.back();
			exceptions.clear();
			stopPlayback();
			throw Exception(e);
		}
	}

	if (running) {
		std::unique_lock<std::mutex> lock(audioMutex);
		if (!outbox.empty()) {
			size_t i = inbox.size();
			inbox.resize(i + outbox.size());
			for (auto& o: outbox) {
				inbox[i++] = std::move(o);
			}
			outbox.clear();
		}

		playingSounds = playingSoundsNext;
	} else {
		inbox.clear();
		outbox.clear();
	}
}
