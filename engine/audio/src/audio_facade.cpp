#include "audio_facade.h"
#include "audio_engine.h"
#include "audio_handle_impl.h"
#include "audio_source_behaviour.h"

using namespace Halley;

constexpr bool ownAudioThread = false;

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
	format.bufferSize = 2048;
	format.format = AudioSampleFormat::Int16;
	format.numChannels = 2;
	format.sampleRate = 48000;

	AudioSpec obtained = output.openAudioDevice(format, getAudioDevices().at(deviceNumber).get(), [this]() { onNeedBuffer(); });

	engine->start(obtained, output);
	running = true;

	if (ownAudioThread) {
		audioThread = std::thread([this]() { run(); });
	}

	output.startPlayback();
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
		engine.reset();
		output.stopPlayback();
	}
}

AudioHandle AudioFacade::playUI(std::shared_ptr<const AudioClip> clip, float volume, float pan, bool loop)
{
	return play(clip, AudioSourcePosition::makeUI(pan), volume, loop);
}

AudioHandle AudioFacade::playMusic(std::shared_ptr<const AudioClip> clip, int track, float fadeInTime, bool loop)
{
	bool hasFade = fadeInTime > 0.0001f;
	
	stopMusic(track, fadeInTime);
	auto handle = play(clip, AudioSourcePosition::makeFixed(), hasFade ? 0.0f : 1.0f, true);
	musicTracks[track] = handle;

	if (hasFade) {
		handle->setBehaviour(std::make_unique<AudioSourceFadeBehaviour>(fadeInTime, 1.0f, false));
	}

	return handle;
}

AudioHandle AudioFacade::play(std::shared_ptr<const AudioClip> clip, AudioSourcePosition position, float volume, bool loop)
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
		handle->setBehaviour(std::make_unique<AudioSourceFadeBehaviour>(fadeOutTime, 0.0f, true));
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
