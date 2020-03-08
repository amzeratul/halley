#pragma once
#include "audio_buffer.h"
#include <atomic>
#include <condition_variable>
#include <map>
#include <vector>
#include "audio_voice.h"
#include "halley/audio/resampler.h"
#include "halley/maths/random.h"

namespace Halley {
	class AudioMixer;
	class IAudioClip;
	class Resources;

    class AudioEngine
    {
    public:
	    AudioEngine();
		~AudioEngine();

	    void postEvent(uint32_t id, const AudioEvent& event, const AudioPosition& position);
	    void play(uint32_t id, std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop);
	    void setListener(AudioListenerData position);
		void setOutputChannels(std::vector<AudioChannelData> channelData);

		void addEmitter(uint32_t id, std::unique_ptr<AudioVoice>&& src);

		const std::vector<AudioVoice*>& getSources(uint32_t id);
		std::vector<uint32_t> getPlayingSounds();

		void run();
		void start(AudioSpec spec, AudioOutputAPI& out);
		void resume();
		void pause();

		void generateBuffer();
	    
    	Random& getRNG();
		AudioBufferPool& getPool() const;

		void setMasterGain(float gain);
		void setGroupGain(const String& name, float gain);
		int getGroupId(const String& group);

    private:
		AudioSpec spec;
		AudioOutputAPI* out = nullptr;
		std::unique_ptr<AudioMixer> mixer;
		std::unique_ptr<AudioBufferPool> pool;
		std::unique_ptr<AudioResampler> outResampler;

		std::atomic<bool> running;
		std::atomic<bool> needsBuffer;

		std::vector<std::unique_ptr<AudioVoice>> emitters;
		std::vector<AudioChannelData> channels;
		
		std::map<uint32_t, std::vector<AudioVoice*>> idToSource;
		std::vector<AudioVoice*> dummyIdSource;

		float masterGain = 1.0f;
		std::vector<String> groupNames;
    	std::vector<float> groupGains;

		AudioListenerData listener;

		Random rng;

		void mixEmitters(size_t numSamples, size_t channels, gsl::span<AudioBuffer*> buffers);
	    void removeFinishedEmitters();
		void clearBuffer(gsl::span<AudioSamplePack> dst);

    	float getGroupGain(uint8_t group) const;
    };
}
