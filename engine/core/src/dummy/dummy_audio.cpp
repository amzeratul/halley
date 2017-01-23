#include "dummy_audio.h"

using namespace Halley;

PluginType DummyAudioPlugin::getType()
{
	return PluginType::AudioOutputAPI;
}

String DummyAudioPlugin::getName()
{
	return "dummyAudio";
}

HalleyAPIInternal* DummyAudioPlugin::createAPI(SystemAPI*)
{
	return new DummyAudioAPI();
}

int DummyAudioPlugin::getPriority() const
{
	return -1;
}

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

void DummyAudioAPI::queueAudio(gsl::span<const AudioSamplePack> data)
{
}

size_t DummyAudioAPI::getQueuedSampleCount() const
{
	return 2048;
}

void DummyAudioAPI::init()
{
}

void DummyAudioAPI::deInit()
{
}
