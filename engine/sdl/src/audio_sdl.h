#pragma once
#include "halley/core/api/halley_api_internal.h"
#include "input_sdl.h"
#include <cstdint>

namespace Halley
{
	class AudioDeviceSDL final : public AudioDevice
	{
	public:
		AudioDeviceSDL(String name);
		String getName() const override;

	private:
		String name;
	};

	class AudioSDL final : public AudioOutputAPIInternal
	{
	public:
		void init() override;
		void deInit() override;

		Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() override;
		AudioSpec openAudioDevice(const AudioSpec& requestedFormat, AudioCallback callback, const AudioDevice* device) override;
		void closeAudioDevice() override;

		void startPlayback() override;
		void stopPlayback() override;

		void lockOutputDevice() override;
		void unlockOutputDevice() override;

		inline void onCallback(gsl::span<AudioSamplePack> bytes) const { callback(bytes); }
	private:
		bool playing = false;
		Uint32 device = 0;
		AudioCallback callback;
	};
}
