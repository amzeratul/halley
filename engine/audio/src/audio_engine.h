#pragma once
#include "audio_buffer.h"
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <map>
#include <vector>
#include "audio_source.h"

namespace Halley {
	class AudioMixer;

    class AudioEngine
    {
    public:
	    AudioEngine();
		~AudioEngine();

	    void play(size_t id, std::shared_ptr<const AudioClip> clip, AudioSourcePosition position, float volume, bool loop);
	    void setListener(AudioListenerData position);

		AudioSource* getSource(size_t id);
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

		std::vector<std::unique_ptr<AudioSource>> sources;
		std::vector<AudioChannelData> channels;

		AudioBuffer backBuffer;
    	std::vector<AudioBuffer> channelBuffers;

		std::map<size_t, AudioSource*> idToSource;

		AudioListenerData listener;

		void addSource(size_t id, std::unique_ptr<AudioSource>&& src);

		void updateSources();
	    void postUpdateSources();

		void mixChannel(size_t channelNum, gsl::span<AudioSamplePack> dst);
    };
}
