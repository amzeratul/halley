#pragma once
#include "halley/core/api/audio_api.h"
#include "audio_buffer.h"
#include <mutex>
#include <atomic>

namespace Halley {
    class AudioEngine : public AudioPlaybackAPI
    {
    public:
	    AudioEngine();

	    AudioCallback getCallback();
	    void playUI(std::shared_ptr<AudioClip> clip, float volume, float pan) override;

		void run();
		void start(AudioSpec spec);
		void stop();

    private:
		AudioSpec spec;

		AudioBuffer backBuffer;

		std::atomic<bool> running;
		std::atomic<bool> needsBuffer;
		std::mutex mutex;
		std::condition_variable backBufferCondition;

		void serviceAudio(gsl::span<AudioSamplePack> dst);
	    void generateBuffer();
    };
}
