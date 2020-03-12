#include "audio_sdl.h"
#include <SDL.h>
#include <cstdint>
#include <utility>
#include "halley/text/string_converter.h"
#include "halley/support/logger.h"

using namespace Halley;

AudioDeviceSDL::AudioDeviceSDL(String name)
	: name(std::move(name))
{
}

String AudioDeviceSDL::getName() const
{
	return name != "" ? name : "Default";
}

AudioSDL::AudioSDL()
{
}

void AudioSDL::init()
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
		throw Exception(String("Exception initializing audio: ") + SDL_GetError(), HalleyExceptions::AudioOutPlugin);
	}
}

void AudioSDL::deInit()
{
	closeAudioDevice();
}

Vector<std::unique_ptr<const AudioDevice>> AudioSDL::getAudioDevices()
{
	Vector<std::unique_ptr<const AudioDevice>> result;
	result.emplace_back(std::make_unique<AudioDeviceSDL>("[Default]"));
	int nDevices = SDL_GetNumAudioDevices(0);
	for (int i = 0; i < nDevices; i++) {
		result.emplace_back(std::make_unique<AudioDeviceSDL>(SDL_GetAudioDeviceName(i, 0)));
	}
	return result;
}

static void sdlCallback(void *userdata, Uint8 * stream, int len)
{
	reinterpret_cast<AudioSDL*>(userdata)->onCallback(stream, len);
}

AudioSpec AudioSDL::openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* dev, AudioCallback callback)
{
	prepareAudioCallback = callback;

	String name = dev ? dev->getName() : "[Default]";
	const char* deviceName = name != "[Default]" ? name.c_str() : nullptr;

	auto f = requestedFormat.format;
	SDL_AudioSpec desired;
	desired.callback = sdlCallback;
	desired.channels = requestedFormat.numChannels;
	desired.freq = requestedFormat.sampleRate;
	desired.samples = requestedFormat.bufferSize;
	desired.format = f == AudioSampleFormat::Int16 ? AUDIO_S16SYS : (f == AudioSampleFormat::Int32 ? AUDIO_S32SYS : AUDIO_F32SYS);
	desired.userdata = this;

	SDL_AudioSpec obtained;

	device = SDL_OpenAudioDevice(deviceName, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if (device == 0) {
		throw Exception("Unable to open audio device \"" + (name != "" ? name : "default") + "\"", HalleyExceptions::AudioOutPlugin);
	}

	AudioSpec result;
	result.format = obtained.format == AUDIO_S16SYS ? AudioSampleFormat::Int16 : (obtained.format == AUDIO_S32SYS ? AudioSampleFormat::Int32 : (obtained.format == AUDIO_F32SYS ? AudioSampleFormat::Float : AudioSampleFormat::Undefined));
	if (result.format == AudioSampleFormat::Undefined) {
		throw Exception("Invalid format returned from audio device: " + toString(int(result.format)), HalleyExceptions::AudioOutPlugin);
	}
	result.bufferSize = obtained.samples;
	result.numChannels = obtained.channels;
	result.sampleRate = obtained.freq;
	outputFormat = result;
	return result;
}

void AudioSDL::closeAudioDevice()
{
	stopPlayback();
	if (device != 0) {
		SDL_CloseAudioDevice(device);
		device = 0;
	}
}

void AudioSDL::startPlayback()
{
	if (!device) {
		throw Exception("Audio not initialised.", HalleyExceptions::AudioOutPlugin);
	}
	if (!playing) {
		SDL_PauseAudioDevice(device, 0);
		playing = true;
	}
}

void AudioSDL::stopPlayback()
{
	if (device && playing) {
		SDL_PauseAudioDevice(device, 1);
		playing = false;
	}
}

bool AudioSDL::needsAudioThread() const
{
	return true;
}

void AudioSDL::onAudioAvailable()
{
}

void AudioSDL::onCallback(unsigned char* stream, int len)
{
	const auto dst = gsl::span<std::byte>(reinterpret_cast<std::byte*>(stream), len);
	getAudioOutputInterface().output(dst, true);
}

bool AudioSDL::needsMoreAudio() const
{
	return getAudioOutputInterface().getAvailable() < getAudioBytesNeeded(outputFormat, 2);
}

