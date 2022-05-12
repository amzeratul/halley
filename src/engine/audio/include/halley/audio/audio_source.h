#pragma once

#include <gsl/span>
#include <array>
#include "halley/core/api/audio_api.h"

namespace Halley
{
	class AudioSource {
	public:
		virtual ~AudioSource() {}

		virtual uint8_t getNumberOfChannels() const = 0;
		virtual size_t getSamplesLeft() const = 0;
		virtual bool isReady() const { return true; }
		virtual bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) = 0;
	};
}
