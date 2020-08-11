#pragma once
#include <thread>
#include <atomic>
#include <vector>
#include "halley/core/api/halley_api_internal.h"
#include <map>

#include "halley/data_structures/ring_buffer.h"

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
		AudioHandle playMusic(const String& eventName, int track = 0, float fadeInTime = 0.0f) override;
		AudioHandle getMusic(int track = 0) override;
		void stopMusic(int track = 0, float fadeOutTime = 0.0f) override;
		void stopAllMusic(float fadeOutTime = 0.0f) override;

		void setMasterVolume(float volume = 1.0f) override;
		void setGroupVolume(const String& groupName, float volume = 1.0f) override;

	    void setOutputChannels(std::vector<AudioChannelData> audioChannelData) override;
	    void setListener(AudioListenerData listener) override;

		void onAudioException(std::exception& e);

		void onSuspend() override;
		void onResume() override;
    	
    private:
		Resources* resources = nullptr;
		AudioOutputAPI& output;
		SystemAPI& system;
		std::unique_ptr<AudioEngine> engine;

		std::thread audioThread;
		std::atomic<bool> running;
		std::atomic<bool> started;
	    AudioSpec audioSpec;
		int lastDeviceNumber = 0;

		RingBuffer<std::function<void()>> commandQueue;
		std::vector<std::function<void()>> inbox;
    	
		RingBuffer<String> exceptions;
		std::vector<uint32_t> playingSounds;
		RingBuffer<std::vector<uint32_t>> playingSoundsQueue;

		std::map<int, AudioHandle> musicTracks;

		uint32_t uniqueId = 0;
		bool ownAudioThread;

		void doStartPlayback(int deviceNumber, bool createEngine);
	    void run();
	    void stepAudio();
	    void enqueue(std::function<void()> action);
		
		void stopMusic(AudioHandle& handle, float fade);

		void onNeedBuffer();
    };

	inline float volumeToGain(float volume)
	{
		// gain = a * exp(volume * -ln(a))
		
		constexpr float a = 0.01f;
		constexpr float b = 4.6051701859880913680359829093687f; // -ln(a)
		const float gain = clamp(a * ::expf(volume * b), 0.0f, 1.0f);
		const float linearRolloff = clamp(volume * 10, 0.0f, 1.0f);
		return gain * linearRolloff;
	}

	inline float gainToVolume(float gain)
	{
		// volume = 1 - ln(gain)/ln(a)
		
		constexpr float b = 0.2171472409516259138255644594583; // -1.0f / ln(a)
		const float volume = std::clamp(1.0f + b * logf(std::max(0.01f, gain)), 0.0f, 1.0f);
		return volume;
	}
}
