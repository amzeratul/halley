#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include "halley/core/api/halley_api_internal.h"

namespace Halley {
	class AudioEngine;
	class AudioHandleImpl;

    class AudioFacade : public AudioAPIInternal
    {
		friend class AudioHandleImpl;

    public:
		explicit AudioFacade(AudioOutputAPI& output);
		~AudioFacade();

		void init() override;
		void deInit() override;

		void pump() override;

		Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() override;
		void startPlayback(int deviceNumber) override;
		void stopPlayback() override;

	    AudioHandle playUI(std::shared_ptr<AudioClip> clip, float volume, float pan, bool loop) override;
	    AudioHandle playWorld(std::shared_ptr<AudioClip> clip, Vector2f position, float volume, bool loop) override;
	    void setListener(AudioListenerData listener) override;

    private:
		AudioOutputAPI& output;
		std::unique_ptr<AudioEngine> engine;

		std::thread audioThread;
		std::mutex audioMutex;
		std::atomic<bool> running;

		std::vector<std::function<void()>> outbox;
		std::vector<std::function<void()>> inbox;
		std::vector<size_t> playingSounds;
		std::vector<size_t> playingSoundsNext;

		size_t uniqueId = 0;

		void run();
		void enqueue(std::function<void()> action);
    };
}
