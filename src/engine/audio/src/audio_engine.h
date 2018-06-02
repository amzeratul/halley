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

namespace Halley {
	class AudioMixer;
	class IAudioClip;
	class Resources;

    class AudioEngine
    {
    public:
	    explicit AudioEngine(Resources& resources);
		~AudioEngine();

	    void postEvent(size_t id, const String& name, const AudioPosition& position);
	    void play(size_t id, std::shared_ptr<const IAudioClip> clip, AudioPosition position, float volume, bool loop, float pitch);
	    void setListener(AudioListenerData position);

		AudioEmitter* getSource(size_t id);
		std::vector<size_t> getPlayingSounds();

		void run();
		void start(AudioSpec spec, AudioOutputAPI& out);
		void stop();

		void generateBuffer();
	    
    	Random& getRNG();
		Resources& getResources() const;

    private:
		Resources& resources;

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
		
		std::map<size_t, AudioEmitter*> idToSource;

		AudioListenerData listener;

		Random rng;

		void addEmitter(size_t id, std::unique_ptr<AudioEmitter>&& src);

		void mixEmitters(size_t numSamples, size_t channels, gsl::span<AudioBuffer*> buffers);
	    void removeFinishedEmitters();
		void clearBuffer(gsl::span<AudioSamplePack> dst);
    };
}
