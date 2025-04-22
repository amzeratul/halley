#include "audio_sdl3.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

using namespace Halley;

AudioDeviceSDL3::AudioDeviceSDL3(SDL_AudioDeviceID id)
	: id(id)
{
    name = id != SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK ? SDL_GetAudioDeviceName(id) : "[Default]";
}

SDL_AudioDeviceID AudioDeviceSDL3::getDeviceId() const
{
    return id;
}

String AudioDeviceSDL3::getName() const
{
	return name;
}

AudioSDL3::AudioSDL3() = default;

void AudioSDL3::init()
{
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		throw Exception(String("Exception initializing audio: ") + SDL_GetError(), HalleyExceptions::AudioOutPlugin);
	}
}

void AudioSDL3::deInit()
{
	closeAudioDevice();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

Vector<std::unique_ptr<const AudioDevice>> AudioSDL3::getAudioDevices()
{
	Vector<std::unique_ptr<const AudioDevice>> result;

    result.emplace_back(std::make_unique<AudioDeviceSDL3>(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK));

    int numDevices;
    SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&numDevices);
    if (devices != nullptr) {
        for (int i = 0; i < numDevices; i++) {
            result.emplace_back(std::make_unique<AudioDeviceSDL3>(devices[i]));
        }
        SDL_free(devices);
    }

	return result;
}

static void sdlCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount)
{
	reinterpret_cast<AudioSDL3*>(userdata)->onCallback(stream, additionalAmount);
}

AudioSpec AudioSDL3::openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* dev, AudioCallback callback)
{
    auto device = dynamic_cast<const AudioDeviceSDL3*>(dev);

    SDL_AudioDeviceID deviceId = device ? device->getDeviceId() : SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
	String name = device ? device->getName() : "[Default]";

	SDL_AudioSpec desired;
	desired.channels = requestedFormat.numChannels;
	desired.freq = requestedFormat.sampleRate;

    if (requestedFormat.format == AudioSampleFormat::Int16) {
        desired.format = SDL_AUDIO_S16;
    } else if (requestedFormat.format == AudioSampleFormat::Int32) {
        desired.format = SDL_AUDIO_S32;
    } else {
        desired.format = SDL_AUDIO_F32;
    }

    stream = SDL_OpenAudioDeviceStream(deviceId, &desired, sdlCallback, this);
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));

    if (stream == nullptr) {
        throw Exception("Unable to open audio device \"" + (name != "" ? name : "default") + "\"", HalleyExceptions::AudioOutPlugin);
    }

    SDL_AudioSpec obtained;
    SDL_GetAudioStreamFormat(stream, nullptr, &obtained);

	AudioSpec result;
    result.bufferSize = requestedFormat.bufferSize;
    result.numChannels = obtained.channels;
    result.sampleRate = obtained.freq;

    if (obtained.format == SDL_AUDIO_S16) {
        result.format = AudioSampleFormat::Int16;
    } else if (obtained.format == SDL_AUDIO_S32) {
        result.format = AudioSampleFormat::Int32;
    } else if (obtained.format == SDL_AUDIO_F32) {
        result.format = AudioSampleFormat::Float;
    } else {
        result.format = AudioSampleFormat::Undefined;
    }

	if (result.format == AudioSampleFormat::Undefined) {
		throw Exception("Invalid format returned from audio device: " + toString(int(result.format)), HalleyExceptions::AudioOutPlugin);
	}

	outputFormat = result;

	return result;
}

void AudioSDL3::closeAudioDevice()
{
	stopPlayback();
	if (stream != nullptr) {
		SDL_DestroyAudioStream(stream);
		stream = nullptr;
	}
}

void AudioSDL3::startPlayback()
{
	if (stream == nullptr) {
		throw Exception("Audio not initialised.", HalleyExceptions::AudioOutPlugin);
	}
	if (!playing) {
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));
		playing = true;
	}
}

void AudioSDL3::stopPlayback()
{
	if (stream != nullptr && playing) {
        SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(stream));
		playing = false;
	}
}

void AudioSDL3::onSuspend()
{
	deInit();
}

void AudioSDL3::onResume()
{
	init();
}

bool AudioSDL3::needsAudioThread() const
{
	return true;
}

void AudioSDL3::onAudioAvailable()
{
}

void AudioSDL3::onCallback(SDL_AudioStream* sdlStream, int len)
{
    if (len > 0) {
        Ensures(sdlStream == stream);
        auto data = SDL_stack_alloc(std::byte, len);
        if (data) {
            const auto dst = gsl::span(data, len);
            if (playing) {
            	getAudioOutputInterface().output(dst, true);
            } else {
	            memset(data, 0, len);
            }
            SDL_PutAudioStreamData(sdlStream, data, len);
        }
    }
}

bool AudioSDL3::needsMoreAudio()
{
	return getAudioOutputInterface().getAvailable() < getAudioBytesNeeded(outputFormat, 2);
}

