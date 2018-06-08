#pragma once
#include "audio_buffer.h"
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <map>
#include <vector>
#include "audio_emitter.h"
#include "halley/audio/resampler.h"
#include "halley/maths/random.h"
#include "halley/data_structures/flat_map.h"

namespace Halley {
	class AudioMixer;
	class IAudioClip;
	class Resources;

    class AudioEngine
    {
    public:
	    AudioEngine();
		~AudioEngine();

	    void postEvent(size_t id, std::shared_ptr<const AudioEvent> event, const AudioPosition& position);
	    void play(size_t id, std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop);
	    void setListener(AudioListenerData position);

		void addEmitter(size_t id, std::unique_ptr<AudioEmitter>&& src);

		const std::vector<AudioEmitter*>& getSources(size_t id);
		std::vector<size_t> getPlayingSounds();

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
		AudioOutputAPI* out;
		std::unique_ptr<AudioMixer> mixer;
		std::unique_ptr<AudioBufferPool> pool;
		std::unique_ptr<AudioResampler> outResampler;

		std::atomic<bool> running;
		std::atomic<bool> needsBuffer;
		std::mutex mutex;
		std::condition_variable backBufferCondition;

		std::vector<std::unique_ptr<AudioEmitter>> emitters;
		std::vector<AudioChannelData> channels;
		
		std::map<size_t, std::vector<AudioEmitter*>> idToSource;
		std::vector<AudioEmitter*> dummyIdSource;

		float masterGain = 1.0f;
		std::vector<String> groupNames;
    	std::vector<float> groupGains;

		AudioListenerData listener;

		Random rng;

		void mixEmitters(size_t numSamples, size_t channels, gsl::span<AudioBuffer*> buffers);
	    void removeFinishedEmitters();
		void clearBuffer(gsl::span<AudioSamplePack> dst);

    	float getGroupGain(int group) const;
    };
}
