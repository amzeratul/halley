#pragma once

#include <gsl/span>
#include <array>
#include "halley/core/api/audio_api.h"

namespace Halley
{
	class AudioSource {
	public:
		virtual ~AudioSource() {}

		virtual size_t getNumberOfChannels() const = 0;
		virtual bool getAudioData(size_t numSamples, std::array<gsl::span<AudioSamplePack>, 8> dst) = 0;
		virtual bool isReady() const { return true; }
	};
}
