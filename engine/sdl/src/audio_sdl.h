#pragma once
#include "halley/core/api/halley_api_internal.h"
#include "input_sdl.h"
#include <cstdint>
#include <vector>

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
		AudioSpec openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* device, AudioCallback prepareAudioCallback) override;
		void closeAudioDevice() override;

		void startPlayback() override;
		void stopPlayback() override;

		void queueAudio(gsl::span<const AudioSamplePack> data) override;
		size_t getQueuedSampleCount() const override;
		void onCallback(unsigned char* stream, int len);

		bool needsAudioThread() override;

	private:
		bool playing = false;
		Uint32 device = 0;
		AudioSpec outputFormat;

		std::vector<short> tmpShort;
		std::vector<int> tmpInt;

		std::mutex mutex;
		std::list<std::vector<unsigned char>> audioQueue;
		size_t readPos = 0;
		size_t queuedSize = 0;

		AudioCallback prepareAudioCallback;

		void doQueueAudio(gsl::span<const gsl::byte> data);
	};
}
