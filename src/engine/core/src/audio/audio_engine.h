#pragma once
#include "halley/audio/audio_buffer.h"
#include "halley/audio/audio_env.h"
#include <atomic>
#include <condition_variable>
#include <map>

#include "audio_emitter.h"
#include "audio_region.h"
#include "halley/data_structures/vector.h"

#include "audio_voice.h"
#include "halley/audio/audio_event.h"
#include "halley/audio/resampler.h"
#include "halley/data_structures/hash_map.h"
#include "halley/data_structures/ring_buffer.h"
#include "halley/maths/random.h"

namespace Halley {
	class AudioRegion;
	class AudioBusProperties;
	class AudioProperties;
	class AudioMixer;
	class IAudioClip;
	class Resources;
	class AudioVariableTable;

	class AudioEngine final: private IAudioOutput, public AudioEnv
    {
    public:
		using VoiceCallback = std::function<void(AudioVoice&)>;
    	
	    AudioEngine();
		~AudioEngine();

		void createEmitter(AudioEmitterId id, AudioPosition position, bool temporary);
		void destroyEmitter(AudioEmitterId id);

		void createRegion(AudioRegionId id);
		void destroyRegion(AudioRegionId id);

	    void postEvent(AudioEventId id, const AudioEvent& event, AudioEmitterId emitterId);
	    void play(AudioEventId id, std::shared_ptr<const IAudioClip> clip, AudioEmitterId emitterId, float gain, bool loop, AudioFade fade);
	    void play(AudioEventId id, std::shared_ptr<const AudioObject> object, AudioEmitterId emitterId, float gain, AudioFade fade);
		
	    void setListener(AudioListenerData position);
		void setOutputChannels(Vector<AudioChannelData> channelData);

		void forVoices(AudioObjectId audioObjectId, VoiceCallback callback);
		void forVoicesOnBus(int busId, VoiceCallback callback);

		AudioEmitter* getEmitter(AudioEmitterId id);
		AudioRegion* getRegion(AudioRegionId id);
		Vector<AudioEventId> getFinishedSounds();

		void run();
		void start(AudioSpec spec, AudioOutputAPI& out, const AudioProperties& audioProperties);
		void resume();
		void pause();

		void generateBuffer();
	    
    	Random& getRNG() override;
		AudioBufferPool& getPool() const override;

		void setMasterGain(float gain);
		void setBusGain(const String& name, float gain);
    	float getCompositeBusGain(uint8_t bus) const;
		int getBusId(const String& busName);
		void getBusIds(const String& busName, Vector<int>& busIds);

		int64_t getLastTimeElapsed();

    	void setBufferSizeController(std::shared_ptr<IAudioBufferSizeController> controller);

    	void setGenerateDebugData(bool enabled);
		std::optional<AudioDebugData> getDebugData() const;

		std::unique_ptr<AudioVoice> makeObjectVoice(const AudioObject& object, AudioEventId uniqueId, AudioEmitter& emitter, Range<float> gain = { 1, 1 }, Range<float> pitch = { 1, 1 }, uint32_t delaySamples = 0);

	private:
		struct BusData {
			String name;
			float gain = 1;
			float compositeGain = 1;
			OptionalLite<uint8_t> parent;
			Vector<uint8_t> children;
		};

		struct PlayingObjectData {
			AudioObjectId id = 0;
			int count = 0;
			float cooldown = 0;
		};

		AudioSpec spec;
		AudioOutputAPI* out = nullptr;
		const AudioProperties* audioProperties = nullptr;
		std::unique_ptr<AudioBufferPool> pool;
		std::unique_ptr<AudioResampler> outResampler;
		Vector<short> tmpShort;
		Vector<int> tmpInt;
		RingBuffer<gsl::byte> audioOutputBuffer;

		std::atomic<bool> running;
		std::atomic<bool> needsBuffer;

		HashMap<AudioEmitterId, std::unique_ptr<AudioEmitter>> emitters;
		HashMap<AudioRegionId, std::unique_ptr<AudioRegion>> regions;
		Vector<AudioChannelData> channels;
		
		float masterGain = 1.0f;
		Vector<BusData> buses;

		AudioListenerData listener;

		Random rng;
		std::atomic<int64_t> lastTimeElapsed;

    	Vector<uint32_t> finishedSounds;
		Vector<PlayingObjectData> playingObjectData;

		std::shared_ptr<IAudioBufferSizeController> bufferSizeController;

		bool debugDataEnabled = false;

		void mixVoices(size_t numSamples, size_t channels, AudioBuffersRef& buffers);
		void mixMainRegion(size_t numSamples, size_t nChannels, AudioRegion& region, AudioBuffersRef& outputBuffers, float prevGain, float gain);
		void mixRegion(const AudioRegion& region, AudioBuffersRef& buffers, float prevGain, float gain);

	    void removeFinishedVoices();
		void queueAudioFloat(gsl::span<const float> data);
		void queueAudioBytes(gsl::span<const gsl::byte> data);
		bool needsMoreAudio();

		size_t getAvailable() override;
		size_t output(gsl::span<std::byte> dst, bool fill) override;

    	void loadBuses();
		uint8_t loadBus(const AudioBusProperties& bus, OptionalLite<uint8_t> parent);
		void updateBusGains();
		void collectBusChildren(Vector<int>& dst, const BusData& bus) const;

		void updateRegions();

		PlayingObjectData& getMutablePlayingObjectData(AudioObjectId id);
		void updatePlayingObjectData(float deltaTime);

		AudioDebugData generateDebugData() const;
    };
}
