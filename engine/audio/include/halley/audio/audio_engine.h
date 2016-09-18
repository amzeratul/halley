#pragma once
#include "halley/core/api/audio_api.h"

namespace Halley {
    class AudioEngine : public AudioAPI
    {
    public:
	    explicit AudioEngine(AudioSpec spec);

	    AudioCallback getCallback() override;
	    void playUI(std::shared_ptr<AudioClip> clip, float volume, float pan) override;

    private:
		AudioSpec spec;


		void serviceAudio(gsl::span<AudioSamplePack> dst);
    };
}
