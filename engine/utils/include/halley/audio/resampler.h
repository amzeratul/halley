#pragma once

#include <gsl/gsl>

struct SpeexResamplerState_;
typedef struct SpeexResamplerState_ SpeexResamplerState;

namespace Halley
{
	class AudioResampler
	{
	public:
		AudioResampler(int from, int to, int nChannels, float quality = 1.0f);
		~AudioResampler();

		void resampleInterleaved(gsl::span<const float> src, gsl::span<float> dst);
		size_t numOutputSamples(size_t numInputSamples) const;

	private:
		SpeexResamplerState* resampler;
	};
}