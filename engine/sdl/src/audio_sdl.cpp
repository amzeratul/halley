#include "audio_sdl.h"
#include <SDL.h>
#include <cstdint>

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

AudioSpec AudioSDL::openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* dev)
{
	String name = dev ? dev->getName() : "";
	const char* deviceName = name != "" ? name.c_str() : nullptr;

	auto f = requestedFormat.format;
	SDL_AudioSpec desired;
	desired.callback = nullptr;
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

	AudioSpec result;
	result.format = obtained.format == AUDIO_S16SYS ? AudioSampleFormat::Int16 : (obtained.format == AUDIO_S32SYS ? AudioSampleFormat::Int32 : (obtained.format == AUDIO_F32SYS ? AudioSampleFormat::Float : AudioSampleFormat::Undefined));
	if (result.format == AudioSampleFormat::Undefined) {
		throw Exception("Invalid format returned from audio device: " + toString(int(result.format)));
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

void AudioSDL::queueAudio(gsl::span<const AudioSamplePack> data)
{
	Expects(device);

	const size_t numSamples = data.size() * 16;

	// Float
	if (outputFormat.format == AudioSampleFormat::Float) {
		SDL_QueueAudio(device, data.data(), uint32_t(data.size_bytes()));
	}
	
	// Int16
	else if (outputFormat.format == AudioSampleFormat::Int16) {
		if (tmpShort.size() < numSamples) {
			tmpShort.resize(numSamples);
		}
		for (ptrdiff_t i = 0; i < data.size(); ++i) {
			for (size_t j = 0; j < 16; ++j) {
				tmpShort[i * 16 + j] = static_cast<short>(data[i].samples[j] * 32768.0f);
			}
		}

		SDL_QueueAudio(device, tmpShort.data(), uint32_t(tmpShort.size() * sizeof(short)));
	}
	
	// Int32
	else if (outputFormat.format == AudioSampleFormat::Int32) {
		if (tmpInt.size() < numSamples) {
			tmpInt.resize(numSamples);
		}
		for (ptrdiff_t i = 0; i < data.size(); ++i) {
			for (size_t j = 0; j < 16; ++j) {
				tmpInt[i * 16 + j] = static_cast<int>(data[i].samples[j] * 2147483648.0f);
			}
		}

		SDL_QueueAudio(device, tmpInt.data(), uint32_t(tmpInt.size() * sizeof(int)));
	}
}

size_t AudioSDL::getQueuedSampleCount() const
{
	Expects(device);
	size_t sizePerSample = outputFormat.format == AudioSampleFormat::Int16 ? 2 : 4;
	return size_t(SDL_GetQueuedAudioSize(device)) / outputFormat.numChannels;
}
