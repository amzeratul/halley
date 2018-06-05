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
	class IAudioClip;

    class AudioFacade final : public AudioAPIInternal
    {
		friend class AudioHandleImpl;

    public:
		explicit AudioFacade(AudioOutputAPI& output, SystemAPI& system);
		~AudioFacade();

		void setResources(Resources& resources) override;

		void init() override;
		void deInit() override;

		void pump() override;

		Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() override;
		void startPlayback(int deviceNumber) override;
		void stopPlayback() override;
		void pausePlayback() override;
		void resumePlayback() override;

	    AudioHandle postEvent(const String& name, AudioPosition position) override;

    	AudioHandle play(std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop) override;
		AudioHandle playMusic(const String& eventName, int track = 0) override;
		AudioHandle getMusic(int track = 0) override;
		void stopMusic(int track = 0, float fadeOutTime = 0.0f) override;
		void stopAllMusic(float fadeOutTime = 0.0f) override;

		void setMasterVolume(float volume = 1.0f) override;
		void setGroupVolume(const String& groupName, float volume = 1.0f) override;

	    void setListener(AudioListenerData listener) override;

		void onAudioException(std::exception& e);

    private:
		Resources* resources;
		AudioOutputAPI& output;
		SystemAPI& system;
		std::unique_ptr<AudioEngine> engine;

		std::thread audioThread;
		std::mutex audioMutex;
		std::mutex exceptionMutex;
		std::atomic<bool> running;
		std::atomic<bool> started;
	    AudioSpec audioSpec;

		std::vector<std::function<void()>> outbox;
		std::vector<std::function<void()>> inbox;
		std::vector<String> exceptions;
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

	inline float volumeToGain(float volume)
	{
		constexpr float a = 0.01f;
		constexpr float b = 4.6051701859880913680359829093687f;
		const float gain = clamp(a * ::expf(volume * b), 0.0f, 1.0f);
		const float linearRolloff = clamp(gain * 10, 0.0f, 1.0f);
		return gain * linearRolloff;
	}

	inline float gainToVolume(float gain)
	{
		constexpr float a = 0.01f;
		constexpr float b = 4.6051701859880913680359829093687f;
		float baseGain = gain;
		if (baseGain < 0.1f) {
			// Undo linear rolloff
			baseGain = sqrt(baseGain * 0.1f);
		}
		return clamp(logf(baseGain / a) / b, 0.0f, 1.0f);
	}
}
