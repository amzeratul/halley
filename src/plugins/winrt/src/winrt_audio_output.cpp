#include "winrt_audio_output.h"
using namespace Halley;

void WinRTAudioOutput::init()
{
}

void WinRTAudioOutput::deInit()
{
}

Vector<std::unique_ptr<const AudioDevice>> WinRTAudioOutput::getAudioDevices()
{
	// TODO
	return {};
}

AudioSpec WinRTAudioOutput::openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* device, AudioCallback prepareAudioCallback)
{
	return requestedFormat;
}

void WinRTAudioOutput::closeAudioDevice()
{
	// TODO
}

void WinRTAudioOutput::startPlayback()
{
	// TODO
}

void WinRTAudioOutput::stopPlayback()
{
	// TODO
}

void WinRTAudioOutput::queueAudio(gsl::span<const float> data)
{
	// TODO
}

bool WinRTAudioOutput::needsMoreAudio()
{
	// TODO
	return false;
}

bool WinRTAudioOutput::needsAudioThread() const
{
	return false;
}
