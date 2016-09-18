#include "audio_facade.h"
#include "audio_engine.h"

using namespace Halley;

AudioFacade::AudioFacade(AudioSpec spec)
	: engine(std::make_unique<AudioEngine>(spec))
	, running(true)
{
	audioThread = std::thread([this] () { run(); });
}

AudioFacade::~AudioFacade()
{
	running = false;
	audioThread.join();
}

AudioCallback AudioFacade::getCallback()
{
	return engine->getCallback();
}

void AudioFacade::playUI(std::shared_ptr<AudioClip> clip, float volume, float pan)
{
	
}

void AudioFacade::run()
{
	
}
