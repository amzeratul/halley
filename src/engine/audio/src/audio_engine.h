#pragma once
#include "audio_buffer.h"
#include <atomic>
#include <condition_variable>
#include <map>
#include "halley/data_structures/vector.h"

#include "audio_voice.h"
#include "halley/audio/resampler.h"
#include "halley/data_structures/ring_buffer.h"
#include "halley/maths/random.h"

namespace Halley {
	class AudioMixer;
	class IAudioClip;
	class Resources;
	class AudioVariableTable;

    class AudioEngine: private IAudioOutput
    {
    public:
	    AudioEngine();
		~AudioEngine();

	    void postEvent(uint32_t id, const AudioEvent& event, const AudioPosition& position);
	    void play(uint32_t id, std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop);
	    void setListener(AudioListenerData position);
		void setOutputChannels(Vector<AudioChannelData> channelData);

		void addEmitter(uint32_t id, std::unique_ptr<AudioVoice> src);

		const Vector<AudioVoice*>& getSources(uint32_t id);
		Vector<uint32_t> getFinishedSounds();

		void run();
		void start(AudioSpec spec, AudioOutputAPI& out);
		void resume();
		void pause();

		void generateBuffer();
	    
    	Random& getRNG();
		AudioBufferPool& getPool() const;
		AudioVariableTable& getVariableTable() const;

		void setMasterGain(float gain);
		void setGroupGain(const String& name, float gain);
		int getGroupId(const String& group);

    	void setVariable(const String& name, float value);

		int64_t getLastTimeElapsed();

    private:
		AudioSpec spec;
		AudioOutputAPI* out = nullptr;
		std::unique_ptr<AudioMixer> mixer;
		std::unique_ptr<AudioBufferPool> pool;
		std::unique_ptr<AudioResampler> outResampler;
		std::unique_ptr<AudioVariableTable> variableTable;
		Vector<short> tmpShort;
		Vector<int> tmpInt;
		RingBuffer<gsl::byte> audioOutputBuffer;

		std::atomic<bool> running;
		std::atomic<bool> needsBuffer;

		Vector<std::unique_ptr<AudioVoice>> emitters;
		Vector<AudioChannelData> channels;
		
		std::map<uint32_t, Vector<AudioVoice*>> idToSource;
		Vector<AudioVoice*> dummyIdSource;

		float masterGain = 1.0f;
		Vector<String> groupNames;
    	Vector<float> groupGains;

		AudioListenerData listener;

		Random rng;
		std::atomic<int64_t> lastTimeElapsed;

    	Vector<uint32_t> finishedSounds;

		void mixEmitters(size_t numSamples, size_t channels, gsl::span<AudioBuffer*> buffers);
	    void removeFinishedEmitters();
		void clearBuffer(gsl::span<AudioSamplePack> dst);
		void queueAudioFloat(gsl::span<const float> data);
		void queueAudioBytes(gsl::span<const gsl::byte> data);
		bool needsMoreAudio();

		size_t getAvailable() override;
		size_t output(gsl::span<std::byte> dst, bool fill) override;

    	float getGroupGain(uint8_t group) const;
    };
}
