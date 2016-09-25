#include "audio_facade.h"
#include "audio_engine.h"

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
	format.bufferSize = 2048;
	format.format = AudioSampleFormat::Float;
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
		running = false;
		engine->stop();
		audioThread.join();
		engine.reset();
		output.stopPlayback();
	}
}

void AudioFacade::playUI(std::shared_ptr<AudioClip> clip, float volume, float pan)
{
	std::unique_lock<std::mutex> lock(audioMutex);

	actions.push_back([=] ()
	{
		engine->playUI(clip, volume, pan);
	});
}

void AudioFacade::playWorld(std::shared_ptr<AudioClip> clip, Vector2f position, float volume)
{
	std::unique_lock<std::mutex> lock(audioMutex);

	actions.push_back([=] ()
	{
		engine->playWorld(clip, position, volume);
	});
}

void AudioFacade::setListener(Vector2f position)
{
	std::unique_lock<std::mutex> lock(audioMutex);

	actions.push_back([=] ()
	{
		engine->setListener(position);
	});
}

void AudioFacade::run()
{
	while (running) {
		std::vector<std::function<void()>> toDo;
		{
			std::unique_lock<std::mutex> lock(audioMutex);
			toDo = std::move(actions);
			actions.clear();
		}

		for (auto& action: toDo) {
			action();
		}

		engine->run();
	}
}
