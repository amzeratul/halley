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
		AudioResampler(float from, float to, int nChannels, float quality = 1.0f);
		~AudioResampler();

		size_t numOutputSamples(size_t numInputSamples) const;

		AudioResamplerResult resample(gsl::span<const float> src, gsl::span<float> dst, size_t channel);
		AudioResamplerResult resampleInterleaved(gsl::span<const float> src, gsl::span<float> dst);
		AudioResamplerResult resampleInterleaved(gsl::span<const short> src, gsl::span<short> dst);
		AudioResamplerResult resampleNonInterleaved(gsl::span<const float> src, gsl::span<float> dst, const size_t numChannels);

		void setFromHz(float from);
		void setToHz(float to);
		void setRate(float from, float to);

	private:
		std::unique_ptr<SpeexResamplerState, void(*)(SpeexResamplerState*)> resampler;
		size_t nChannels = 0;
		float from = 0;
		float to = 0;
		bool dirty = false;

		void applyPendingSampleRate();
	};
	
}