#pragma once

#include <gsl/gsl>
#include <memory>

struct SpeexResamplerState_;
typedef struct SpeexResamplerState_ SpeexResamplerState;

namespace Halley
{
	struct AudioResamplerResult
	{
		size_t nRead;
		size_t nWritten;
	};

	class AudioResampler
	{
	public:
		AudioResampler(int from, int to, int nChannels, float quality = 1.0f);
		~AudioResampler();

		AudioResamplerResult resampleInterleaved(gsl::span<const float> src, gsl::span<float> dst);
		size_t numOutputSamples(size_t numInputSamples) const;

	private:
		std::unique_ptr<SpeexResamplerState, void(*)(SpeexResamplerState*)> resampler;
		size_t nChannels;
		int from;
		int to;
	};
}