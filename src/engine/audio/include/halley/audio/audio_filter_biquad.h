#pragma once

#include "audio_source.h"

namespace Halley {
    class AudioFilterBiquad : public AudioSource {
    public:
		AudioFilterBiquad(std::shared_ptr<AudioSource> src);
		void setParameters(float a0, float a1, float a2, float b1, float b2);
    	
	    uint8_t getNumberOfChannels() const override;
	    bool isReady() const override;
	    bool getAudioData(size_t numSamples, AudioSourceData& dst) override;

    private:
		std::shared_ptr<AudioSource> src;
    };
}
