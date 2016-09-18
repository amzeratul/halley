#pragma once
#include "halley/core/api/audio_api.h"
#include <thread>
#include <mutex>
#include <atomic>

namespace Halley {
	class AudioEngine;

    class AudioFacade : public AudioAPI
    {
    public:
		explicit AudioFacade(AudioSpec spec);
		~AudioFacade();

	    AudioCallback getCallback() override;
	    void playUI(std::shared_ptr<AudioClip> clip, float volume, float pan) override;

    private:
		std::unique_ptr<AudioEngine> engine;

		std::thread audioThread;
		std::mutex audioMutex;
		std::atomic<bool> running;

		void run();
    };
}
