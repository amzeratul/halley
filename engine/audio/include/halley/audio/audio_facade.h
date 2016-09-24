#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include "halley/core/api/halley_api_internal.h"

namespace Halley {
	class AudioEngine;

    class AudioFacade : public AudioAPIInternal
    {
    public:
		explicit AudioFacade(AudioOutputAPI& output);
		~AudioFacade();

		void init() override;
		void deInit() override;

		Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() override;
		void startPlayback(int deviceNumber) override;
		void stopPlayback() override;

	    void playUI(std::shared_ptr<AudioClip> clip, float volume, float pan) override;
	    void playWorld(std::shared_ptr<AudioClip> clip, Vector2f position, float volume) override;
	    void setListener(Vector2f position) override;

    private:
		AudioOutputAPI& output;
		std::unique_ptr<AudioEngine> engine;

		std::thread audioThread;
		std::mutex audioMutex;
		std::atomic<bool> running;

		std::vector<std::function<void()>> actions;

		void run();
    };
}
