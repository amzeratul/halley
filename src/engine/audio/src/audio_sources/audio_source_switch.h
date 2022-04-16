#pragma once
#include "audio_source.h"

namespace Halley
{
	class AudioSourceSwitch final : public AudioSource
	{
	public:
		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		bool isReady() const override;
	};
}
