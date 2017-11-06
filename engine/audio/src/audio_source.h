#pragma once

#include <gsl/span>
#include <array>
#include "halley/core/api/audio_api.h"

namespace Halley
{
	using AudioSourceData = std::array<gsl::span<AudioConfig::SampleFormat>, AudioConfig::maxChannels>;

	class AudioSource {
	public:
		virtual ~AudioSource() {}

		virtual size_t getNumberOfChannels() const = 0;
		virtual bool isReady() const { return true; }
		virtual bool getAudioData(size_t numSamples, AudioSourceData& dst) = 0;
	};
}
