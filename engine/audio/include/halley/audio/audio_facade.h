#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include "halley/core/api/halley_api_internal.h"
#include <map>

namespace Halley {
	class AudioPosition;
	class AudioEngine;
	class AudioHandleImpl;

    class AudioFacade final : public AudioAPIInternal
    {
		friend class AudioHandleImpl;

    public:
		explicit AudioFacade(AudioOutputAPI& output, SystemAPI& system);
		~AudioFacade();

		void init() override;
		void deInit() override;

		void pump() override;

		Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() override;
		void startPlayback(int deviceNumber) override;
		void stopPlayback() override;

    	AudioHandle play(std::shared_ptr<const AudioClip> clip, AudioPosition position, float volume, bool loop) override;
	    AudioHandle playUI(std::shared_ptr<const AudioClip> clip, float volume, float pan, bool loop) override;

		AudioHandle playMusic(std::shared_ptr<const AudioClip> clip, int track = 0, float fadeInTime = 0.0f, bool loop = true) override;
		AudioHandle getMusic(int track = 0) override;
		void stopMusic(int track = 0, float fadeOutTime = 0.0f) override;
		void stopAllMusic(float fadeOutTime = 0.0f) override;

		void setGroupVolume(String groupName, float gain = 1.0f) override;

	    void setListener(AudioListenerData listener) override;

    private:
		AudioOutputAPI& output;
		SystemAPI& system;
		std::unique_ptr<AudioEngine> engine;

		std::thread audioThread;
		std::mutex audioMutex;
		std::atomic<bool> running;

		std::vector<std::function<void()>> outbox;
		std::vector<std::function<void()>> inbox;
		std::vector<size_t> playingSounds;
		std::vector<size_t> playingSoundsNext;

		std::map<int, AudioHandle> musicTracks;

		size_t uniqueId = 0;
		bool ownAudioThread;

		void run();
	    void stepAudio();
	    void enqueue(std::function<void()> action);
		
		void stopMusic(AudioHandle& handle, float fade);

		void onNeedBuffer();
    };
}
