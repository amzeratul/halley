#include "audio_sdl.h"
#include <SDL.h>

using namespace Halley;

AudioDeviceSDL::AudioDeviceSDL(String name)
	: name(name)
{
}

String AudioDeviceSDL::getName() const
{
	return name != "" ? name : "Default";
}

void AudioSDL::init()
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
		throw Exception(String("Exception initializing audio: ") + SDL_GetError());
	}
}

void AudioSDL::deInit()
{
	closeAudioDevice();
}

Vector<std::unique_ptr<const AudioDevice>> AudioSDL::getAudioDevices()
{
	Vector<std::unique_ptr<const AudioDevice>> result;
	int nDevices = SDL_GetNumAudioDevices(0);
	if (nDevices == 0) {
		result.emplace_back(std::make_unique<AudioDeviceSDL>(""));
	} else {
		for (int i = 0; i < nDevices; i++) {
			result.emplace_back(std::make_unique<AudioDeviceSDL>(SDL_GetAudioDeviceName(i, 0)));
		}
	}
	return std::move(result);
}

static void sdlCallback(void* user, uint8_t* bytes, int size)
{
	Expects(size % 64 == 0);
	reinterpret_cast<AudioSDL*>(user)->onCallback(gsl::span<AudioSamplePack>(reinterpret_cast<AudioSamplePack*>(bytes), size / 64));
}

AudioSpec AudioSDL::openAudioDevice(const AudioSpec& requestedFormat, AudioCallback c, const AudioDevice* dev)
{
	String name = dev ? dev->getName() : "";
	const char* deviceName = name != "" ? name.c_str() : nullptr;

	auto f = requestedFormat.format;
	SDL_AudioSpec desired;
	desired.callback = &sdlCallback;
	desired.channels = requestedFormat.numChannels;
	desired.freq = requestedFormat.sampleRate;
	desired.samples = requestedFormat.bufferSize;
	desired.format = f == AudioSampleFormat::Int16 ? AUDIO_S16SYS : (f == AudioSampleFormat::Int32 ? AUDIO_S32SYS : AUDIO_F32SYS);
	desired.userdata = this;

	SDL_AudioSpec obtained;

	device = SDL_OpenAudioDevice(deviceName, 0, &desired, &obtained, 1);
	if (device == 0) {
		throw Exception("Unable to open audio device \"" + (name != "" ? name : "default") + "\"");
	}
	callback = c;

	AudioSpec result;
	result.format = obtained.format == AUDIO_S16SYS ? AudioSampleFormat::Int16 : (obtained.format == AUDIO_S32SYS ? AudioSampleFormat::Int32 : (obtained.format == AUDIO_F32SYS ? AudioSampleFormat::Float : AudioSampleFormat::Undefined));
	if (result.format == AudioSampleFormat::Undefined) {
		throw Exception("Invalid format returned from audio device: " + toString(int(result.format)));
	}
	result.bufferSize = obtained.samples;
	result.numChannels = obtained.channels;
	result.sampleRate = obtained.freq;
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
		throw Exception("Audio not initialised.");
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
