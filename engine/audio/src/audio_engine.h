#pragma once
#include "audio_buffer.h"
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <map>
#include <vector>
#include "audio_emitter.h"

namespace Halley {
	class AudioMixer;

    class AudioEngine
    {
    public:
	    AudioEngine();
		~AudioEngine();

	    void play(size_t id, std::shared_ptr<const AudioClip> clip, AudioPosition position, float volume, bool loop);
	    void setListener(AudioListenerData position);

		AudioEmitter* getSource(size_t id);
		std::vector<size_t> getPlayingSounds();

		void run();
		void start(AudioSpec spec, AudioOutputAPI& out);
		void stop();

		void generateBuffer();

   	private:
		AudioSpec spec;
		AudioOutputAPI* out;
		std::unique_ptr<AudioMixer> mixer;
		std::unique_ptr<AudioBufferPool> pool;

		std::atomic<bool> running;
		std::atomic<bool> needsBuffer;
		std::mutex mutex;
		std::condition_variable backBufferCondition;

		std::vector<std::unique_ptr<AudioEmitter>> emitters;
		std::vector<AudioChannelData> channels;

		AudioBuffer backBuffer;
    	std::vector<AudioBuffer> channelBuffers;

		std::map<size_t, AudioEmitter*> idToSource;

		AudioListenerData listener;

		void addSource(size_t id, std::unique_ptr<AudioEmitter>&& src);

		void mixEmitters();
	    void removeFinishedEmitters();
		void clearBuffer(gsl::span<AudioSamplePack> dst);
    };
}
