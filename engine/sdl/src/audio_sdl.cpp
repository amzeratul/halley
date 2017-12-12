#include "audio_sdl.h"
#include <SDL.h>
#include <cstdint>
#include "halley/text/string_converter.h"
#include "halley/support/logger.h"

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

bool AudioSDL::needsAudioThread() const
{
	return false;
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

	device = SDL_OpenAudioDevice(deviceName, 0, &desired, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);
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

void AudioSDL::queueAudio(gsl::span<const float> data)
{
	Expects(device);

	const size_t numSamples = data.size();

	// Float
	if (outputFormat.format == AudioSampleFormat::Float) {
		doQueueAudio(gsl::as_bytes(data));
	}
	
	// Int16
	else if (outputFormat.format == AudioSampleFormat::Int16) {
		if (tmpShort.size() < numSamples) {
			tmpShort.resize(numSamples);
		}
		for (ptrdiff_t i = 0; i < data.size(); ++i) {
			tmpShort[i] = static_cast<short>(data[i] * 32768.0f);
		}

		doQueueAudio(gsl::as_bytes(gsl::span<short>(tmpShort)));
	}
	
	// Int32
	else if (outputFormat.format == AudioSampleFormat::Int32) {
		if (tmpInt.size() < numSamples) {
			tmpInt.resize(numSamples);
		}
		for (ptrdiff_t i = 0; i < data.size(); ++i) {
			tmpInt[i] = static_cast<int>(data[i] * 2147483648.0f);
		}

		doQueueAudio(gsl::as_bytes(gsl::span<int>(tmpInt)));
	}
}

bool AudioSDL::needsMoreAudio()
{
	/*
	size_t sizePerSample = outputFormat.format == AudioSampleFormat::Int16 ? 2 : 4;
	size_t queuedAudioSize = queuedSize / (outputFormat.numChannels * sizePerSample);
	 */

	// Doesn't use threaded audio
	return false;
}

void AudioSDL::doQueueAudio(gsl::span<const gsl::byte> data) 
{
	std::vector<unsigned char> tmp(data.size_bytes());
	memcpy(tmp.data(), data.data(), data.size_bytes());

	std::unique_lock<std::mutex> lock(mutex);
	queuedSize += data.size_bytes();
	audioQueue.push_back(std::move(tmp));
}

void AudioSDL::onCallback(unsigned char* stream, int len) 
{
	size_t remaining = size_t(len);
	size_t pos = 0;

	while (remaining > 0) {
		std::unique_lock<std::mutex> lock(mutex);
		if (audioQueue.empty()) {
			lock.unlock();
			if (prepareAudioCallback) {
				prepareAudioCallback();
			}
			
			// Either no callback, or callback didn't add anything
			if (audioQueue.empty()) {
				break;
			}
		} else {
			auto& front = audioQueue.front();
			size_t toCopy = std::min(remaining, front.size() - readPos);

			memcpy(stream + pos, front.data() + readPos, toCopy);

			remaining -= toCopy;
			queuedSize -= toCopy;
			pos += toCopy;
			readPos += toCopy;

			if (readPos == front.size()) {
				readPos = 0;
				audioQueue.pop_front();
			}
		}
	}

	if (remaining > 0) {
		// :(
		Logger::logWarning("Insufficient audio data, padding with zeroes.");
		memset(stream + pos, 0, remaining);
	}
}

