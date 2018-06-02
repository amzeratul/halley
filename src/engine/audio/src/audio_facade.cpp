#include "audio_facade.h"
#include "audio_engine.h"
#include "audio_handle_impl.h"
#include "audio_emitter_behaviour.h"
#include "halley/support/console.h"

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
	if (running) {
		stopPlayback();
	}

	auto devices = getAudioDevices();
	if (int(devices.size()) > deviceNumber) {
		engine = std::make_unique<AudioEngine>(*resources);

		AudioSpec format;
		format.bufferSize = 1024;
		format.format = AudioSampleFormat::Float;
		format.numChannels = 2;
		format.sampleRate = 48000;
		
		AudioSpec obtained = output.openAudioDevice(format, devices.at(deviceNumber).get(), [this]() { onNeedBuffer(); });
		
		engine->start(obtained, output);
		running = true;

		if (ownAudioThread) {
			audioThread = system.createThread("Audio", [this]() { run(); });
		}

		output.startPlayback();

		std::cout << "Audio Playback started.\n";
		std::cout << "\tDevice: " << devices.at(deviceNumber)->getName() << " [" << deviceNumber << "]\n";
		std::cout << "\tSample rate: " << obtained.sampleRate << "\n";
		std::cout << "\tChannels: " << obtained.numChannels << "\n";
		std::cout << "\tFormat: " << toString(obtained.format) << "\n";
		std::cout << "\tBuffer size: " << obtained.bufferSize << std::endl;
	}
}

void AudioFacade::stopPlayback()
{
	if (running) {
		{
			std::unique_lock<std::mutex> lock(audioMutex);
			musicTracks.clear();
			running = false;
			engine->stop();
		}
		if (ownAudioThread) {
			audioThread.join();
		}
		output.stopPlayback();
		engine.reset();
	}
}

AudioHandle AudioFacade::postEvent(const String& name, AudioPosition position)
{
	size_t id = uniqueId++;
	enqueue([=] () {
		engine->postEvent(id, name, position);
	});
	return std::make_shared<AudioHandleImpl>(*this, id);
}

AudioHandle AudioFacade::playMusic(std::shared_ptr<const IAudioClip> clip, int track, float fadeInTime, bool loop)
{
	bool hasFade = fadeInTime > 0.0001f;
	
	stopMusic(track, fadeInTime);
	auto handle = play(clip, AudioPosition::makeFixed(), hasFade ? 0.0f : 1.0f, true);
	musicTracks[track] = handle;

	if (hasFade) {
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

void AudioFacade::setGroupVolume(String groupName, float gain)
{
	// TODO
}

void AudioFacade::setListener(AudioListenerData listener)
{
	enqueue([=] () {
		engine->setListener(listener);
	});
}

void AudioFacade::run()
{
	while (running) {
		stepAudio();
	}
}

void AudioFacade::stepAudio()
{
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
}

void AudioFacade::enqueue(std::function<void()> action)
{
	if (running) {
		outbox.push_back(action);
	}
}

void AudioFacade::pump()
{
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
