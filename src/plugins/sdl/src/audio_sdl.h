#pragma once
#include "halley/core/api/halley_api_internal.h"
#include "input_sdl.h"
#include <cstdint>
#include <vector>

#include "halley/data_structures/ring_buffer.h"

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
		AudioSDL();
		void init() override;
		void deInit() override;

		Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() override;
		AudioSpec openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* device, AudioCallback prepareAudioCallback) override;
		void closeAudioDevice() override;

		void startPlayback() override;
		void stopPlayback() override;

		void queueAudio(gsl::span<const float> data) override;
		bool needsMoreAudio() override;
		void onCallback(unsigned char* stream, int len);

		bool needsAudioThread() const override;

	private:
		bool playing = false;
		Uint32 device = 0;
		AudioSpec outputFormat;

		std::vector<short> tmpShort;
		std::vector<int> tmpInt;

		RingBuffer<gsl::byte> ringBuffer;
		
		AudioCallback prepareAudioCallback;

		void doQueueAudio(gsl::span<const gsl::byte> data);
	};
}
