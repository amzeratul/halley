#include "audio_facade.h"
#include "audio_engine.h"
#include "audio_handle_impl.h"

using namespace Halley;

AudioFacade::AudioFacade(AudioOutputAPI& output)
	: output(output)
	, running(false)
{
}

AudioFacade::~AudioFacade()
{
	AudioFacade::stopPlayback();
}

void AudioFacade::init()
{
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

	engine = std::make_unique<AudioEngine>();

	AudioSpec format;
	format.bufferSize = 1024;
	format.format = AudioSampleFormat::Int16;
	format.numChannels = 2;
	format.sampleRate = 48000;

	AudioSpec obtained = output.openAudioDevice(format, getAudioDevices().at(deviceNumber).get());

	engine->start(obtained, output);
	running = true;
	audioThread = std::thread([this] () { run(); });

	output.startPlayback();
}

void AudioFacade::stopPlayback()
{
	if (running) {
		musicTracks.clear();
		running = false;
		engine->stop();
		audioThread.join();
		engine.reset();
		output.stopPlayback();
	}
}

AudioHandle AudioFacade::playUI(std::shared_ptr<AudioClip> clip, float volume, float pan, bool loop)
{
	size_t id = uniqueId++;
	enqueue([=] () {
		engine->playUI(id, clip, volume, pan, loop);
	});
	return std::make_shared<AudioHandleImpl>(*this, id);
}

AudioHandle AudioFacade::playWorld(std::shared_ptr<AudioClip> clip, Vector2f position, float volume, bool loop)
{
	size_t id = uniqueId++;
	enqueue([=] () {
		engine->playWorld(id, clip, position, volume, loop);
	});
	return std::make_shared<AudioHandleImpl>(*this, id);
}

AudioHandle AudioFacade::playMusic(std::shared_ptr<AudioClip> clip, int track, float fadeInTime, bool loop)
{
	// TODO: fade in
	stopMusic(track, fadeInTime);
	auto handle = playUI(clip, 1.0f, 0.5f, true);
	musicTracks[track] = handle;
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
	// TODO: fade out
	auto iter = musicTracks.find(track);
	if (iter != musicTracks.end()) {
		auto& handle = iter->second;
		if (handle) {
			handle->stop();
			handle.reset();
		}
	}
}

void AudioFacade::stopAllMusic(float fadeOutTime)
{
	// TODO: fade out
	for (auto& m: musicTracks) {
		m.second->stop();
	}
	musicTracks.clear();
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
		std::vector<std::function<void()>> toDo;
		{
			std::unique_lock<std::mutex> lock(audioMutex);
			toDo = std::move(inbox);
			inbox.clear();
			playingSoundsNext = engine->getPlayingSounds();
		}

		for (auto& action: toDo) {
			action();
		}

		engine->run();
	}
}

void AudioFacade::enqueue(std::function<void()> action)
{
	outbox.push_back(action);
}

void AudioFacade::pump()
{
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
}
