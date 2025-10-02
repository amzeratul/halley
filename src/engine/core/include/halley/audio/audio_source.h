#pragma once

#include <gsl/span>
#include <array>
#include "halley/api/audio_api.h"

namespace Halley
{
	class AudioSource {
	public:
		virtual ~AudioSource() {}

		virtual String getName() const = 0;
		virtual uint8_t getNumberOfChannels() const = 0;
		virtual size_t getSamplesLeft() const = 0;
		virtual bool isReady() const = 0;
		virtual bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) = 0;
		virtual void restart() = 0;
		virtual bool isLooping() = 0;
	};
}
