#pragma once
#include "audio_buffer.h"

namespace Halley {
	class AudioBuffersRef;

	class AudioFilterBiquad {
    public:
		void setParameters(float a0, float a1, float a2, float b0, float b1, float b2);
		void setParameters(float a1, float a2, float b0, float b1, float b2);
		void setLowPass(float cutoffHz, float sampleRate = 48000.0f);

        float processSample(float sample, size_t channelNumber);
		void processSamples(AudioBuffersRef& buffers);
		void processSamples(AudioBuffer& buffer, size_t channelNumber);

	private:
		float a1 = 0;
		float a2 = 0;
		float b0 = 0;
		float b1 = 0;
		float b2 = 0;

		struct ChannelData {
			float x1 = 0;
			float x2 = 0;
			float y1 = 0;
			float y2 = 0;
		};
		std::array<ChannelData, AudioConfig::maxChannels> channels;
    };
}
