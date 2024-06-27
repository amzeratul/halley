#pragma once
#include <thread>
#include <atomic>
#include "halley/data_structures/vector.h"
#include "halley/api/halley_api_internal.h"
#include <map>

#include "halley/data_structures/ring_buffer.h"

namespace Halley {
	class AudioObject;
	class AudioProperties;
	class AudioPosition;
	class AudioEngine;
	class AudioHandleImpl;
	class AudioEmitterHandleImpl;
	class IAudioClip;

    class AudioFacade final : public AudioAPIInternal
    {
		friend class AudioHandleImpl;
		friend class AudioEmitterHandleImpl;
		friend class AudioRegionHandleImpl;

    public:
		explicit AudioFacade(AudioOutputAPI& output, SystemAPI& system);
		~AudioFacade() override;

		void setResources(Resources& resources) override;

		void init() override;
		void deInit() override;

		void pump() override;

		Vector<std::unique_ptr<const AudioDevice>> getAudioDevices() override;
		void startPlayback(int deviceNumber) override;
		void stopPlayback() override;
		void pausePlayback() override;
		void resumePlayback() override;

		AudioEmitterHandle createEmitter(AudioPosition position) override;
		AudioEmitterHandle getGlobalEmitter() override;

		AudioRegionHandle createRegion(const String& name) override;
		String getRegionName(AudioRegionId id) override;

		AudioHandle postEvent(const String& name) override;
	    AudioHandle postEvent(const String& name, AudioEmitterHandle emitter) override;
		AudioHandle postEvent(const AudioEvent& event) override;
		AudioHandle postEvent(const AudioEvent& event, AudioEmitterHandle emitter) override;
		AudioHandle play(std::shared_ptr<const IAudioClip> clip, AudioEmitterHandle emitter, float volume, bool loop) override;
		AudioHandle play(std::shared_ptr<const AudioObject> audioObject, AudioEmitterHandle emitter, float volume) override;

    	AudioHandle postEvent(const String& name, AudioPosition position) override;
    	AudioHandle play(std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop) override;
		AudioHandle playMusic(const String& eventName, int track = 0, float fadeInTime = 0.0f) override;
		AudioHandle getMusic(int track = 0) override;
		void stopMusic(int track = 0, float fadeOutTime = 0.0f) override;
		void stopAllMusic(float fadeOutTime = 0.0f) override;

		void setMasterVolume(float volume = 1.0f) override;
		void setBusVolume(const String& busName, float volume = 1.0f) override;

	    void setOutputChannels(Vector<AudioChannelData> audioChannelData) override;
	    void setListener(AudioListenerData listener) override;

		void onAudioException(std::exception& e);

		void onSuspend() override;
		void onResume() override;

		int64_t getLastTimeElapsed() const override;
		std::optional<AudioSpec> getAudioSpec() const override;

		void setBufferSizeController(std::shared_ptr<IAudioBufferSizeController> controller) override;

		void setDebugListener(IAudioDebugDataListener* listener) override;

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

		RingBuffer<Vector<std::function<void()>>> commandQueue;
		Vector<std::function<void()>> outbox;
		Vector<std::function<void()>> inbox;
    	
		RingBuffer<String> exceptions;
		Vector<uint32_t> playingSounds;
		RingBuffer<Vector<uint32_t>> finishedSoundsQueue;

		std::map<int, AudioHandle> musicTracks;

		AudioEventId curEventId = 0;
		bool ownAudioThread;

		AudioEmitterId curEmitterId = 1;
		AudioRegionId curRegionId = 1;

		RingBuffer<AudioDebugData> audioDebugData;
		IAudioDebugDataListener* debugListener = nullptr;

		HashMap<AudioRegionId, String> regionNames;

		AudioHandle doPostEvent(const AudioEvent& event, AudioEmitterId emitterId);
		AudioHandle doPostEvent(const String& name, AudioEmitterId emitterId);

    	void doStartPlayback(int deviceNumber, bool createEngine);
	    void run();
	    void stepAudio();
	    void enqueue(std::function<void()> action);
		
		void stopMusic(AudioHandle& handle, float fade);

		void onNeedBuffer();

		const AudioProperties& getAudioProperties() const;
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
