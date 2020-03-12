#include "dummy_audio.h"

using namespace Halley;

Vector<std::unique_ptr<const AudioDevice>> DummyAudioAPI::getAudioDevices()
{
	return {};
}

AudioSpec DummyAudioAPI::openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* device, AudioCallback prepareAudioCallback)
{
	return requestedFormat;
}

void DummyAudioAPI::closeAudioDevice()
{
}

void DummyAudioAPI::startPlayback()
{
}

void DummyAudioAPI::stopPlayback()
{
}

void DummyAudioAPI::init()
{
}

void DummyAudioAPI::deInit()
{
}

bool DummyAudioAPI::needsMoreAudio()
{
	return false;
}

bool DummyAudioAPI::needsAudioThread() const
{
	return false;
}

void DummyAudioAPI::onAudioAvailable()
{
}
