#pragma once
#include "halley/audio/audio_buffer.h"
#include <atomic>
#include <condition_variable>
#include <map>

#include "audio_emitter.h"
#include "halley/data_structures/vector.h"

#include "audio_voice.h"
#include "halley/audio/resampler.h"
#include "halley/data_structures/hash_map.h"
#include "halley/data_structures/ring_buffer.h"
#include "halley/maths/random.h"

namespace Halley {
	class AudioBusProperties;
	class AudioProperties;
	class AudioMixer;
	class IAudioClip;
	class Resources;
	class AudioVariableTable;

    class AudioEngine final: private IAudioOutput
    {
    public:
		using VoiceCallback = std::function<void(AudioVoice&)>;
    	
	    AudioEngine();
		~AudioEngine();

		void createEmitter(AudioEmitterId id, AudioPosition position, bool temporary);
		void destroyEmitter(AudioEmitterId id);
		void setEmitterPosition(AudioEmitterId id, AudioPosition position);

	    void postEvent(AudioEventId id, const AudioEvent& event, AudioEmitterId emitterId);
	    void play(AudioEventId id, std::shared_ptr<const IAudioClip> clip, AudioEmitterId emitterId, float volume, bool loop);
		
	    void setListener(AudioListenerData position);
		void setOutputChannels(Vector<AudioChannelData> channelData);

		void forVoices(AudioObjectId audioObjectId, VoiceCallback callback);
		void forVoicesOnBus(int busId, VoiceCallback callback);

		AudioEmitter* getEmitter(AudioEmitterId id);
		Vector<AudioEventId> getFinishedSounds();

		void run();
		void start(AudioSpec spec, AudioOutputAPI& out, const AudioProperties& audioProperties);
		void resume();
		void pause();

		void generateBuffer();
	    
    	Random& getRNG();
		AudioBufferPool& getPool() const;

		void setMasterGain(float gain);
		void setBusGain(const String& name, float gain);
    	float getCompositeBusGain(uint8_t bus) const;
		int getBusId(const String& busName);

		int64_t getLastTimeElapsed();

    private:
		struct BusData {
			String name;
			float gain = 1;
			float compositeGain = 1;
			OptionalLite<uint8_t> parent;
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
		Vector<AudioChannelData> channels;
		
		float masterGain = 1.0f;
		Vector<BusData> buses;

		AudioListenerData listener;

		Random rng;
		std::atomic<int64_t> lastTimeElapsed;

    	Vector<uint32_t> finishedSounds;

		void mixVoices(size_t numSamples, size_t channels, gsl::span<AudioBuffer*> buffers);
	    void removeFinishedVoices();
		void queueAudioFloat(gsl::span<const float> data);
		void queueAudioBytes(gsl::span<const gsl::byte> data);
		bool needsMoreAudio();

		size_t getAvailable() override;
		size_t output(gsl::span<std::byte> dst, bool fill) override;

    	void loadBuses();
		void loadBus(const AudioBusProperties& bus, OptionalLite<uint8_t> parent);
		void updateBusGains();
    };
}
