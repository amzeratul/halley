#pragma once
#include "halley/core/api/halley_api_internal.h"

namespace Halley
{
	class WinRTAudioOutput : public AudioOutputAPIInternal
	{
	public:
		void init() override;
		void deInit() override;

		Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() override;
		AudioSpec openAudioDevice(const AudioSpec& requestedFormat, const AudioDevice* device, AudioCallback prepareAudioCallback) override;
		void closeAudioDevice() override;

		void startPlayback() override;
		void stopPlayback() override;

		void queueAudio(gsl::span<const float> data) override;
		bool needsMoreAudio() override;

		bool needsAudioThread() const override;
	};
}
