#pragma once
#include "halley/api/halley_api_internal.h"
#include <SDL3/SDL.h>

namespace Halley
{
	class AudioDeviceSDL3 final : public AudioDevice
	{
	public:
		explicit AudioDeviceSDL3(SDL_AudioDeviceID id);

        SDL_AudioDeviceID getDeviceId() const;
        String getName() const override;

	private:
        SDL_AudioDeviceID id;
		String name;
	};

	class AudioSDL3 final : public AudioOutputAPIInternal
	{
	public:
		AudioSDL3();
		void init() override;
		void deInit() override;

		Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() override;
		AudioSpec openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* device, AudioCallback prepareAudioCallback) override;
		void closeAudioDevice() override;

		void startPlayback() override;
		void stopPlayback() override;
		void onSuspend() override;
		void onResume() override;
		
		void onCallback(SDL_AudioStream* stream, int len);

		bool needsMoreAudio() override;
		bool needsAudioThread() const override;
		void onAudioAvailable() override;
		
	private:
		bool playing = false;
        SDL_AudioStream* stream = nullptr;
		AudioSpec outputFormat;
	};
}
