#pragma once
#include "audio_buffer.h"
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "audio_source.h"

namespace Halley {
	class AudioMixer;

    class AudioEngine
    {
    public:
	    AudioEngine();
		~AudioEngine();

    	void playUI(std::shared_ptr<AudioClip> clip, float volume, float pan, bool loop);
	    void playWorld(std::shared_ptr<AudioClip> clip, Vector2f position, float volume, bool loop);
	    void setListener(Vector2f position);

		void run();
		void start(AudioSpec spec, AudioOutputAPI& out);
		void stop();

    private:
		AudioSpec spec;
		AudioOutputAPI* out;
		std::unique_ptr<AudioMixer> mixer;

		std::atomic<bool> running;
		std::atomic<bool> needsBuffer;
		std::mutex mutex;
		std::condition_variable backBufferCondition;

		std::vector<std::unique_ptr<AudioSource>> sources;
		std::vector<AudioChannelData> channels;

		AudioBuffer backBuffer;
		AudioBuffer tmpBuffer;
    	std::vector<AudioBuffer> channelBuffers;

	    void generateBuffer();
		void updateSources();
	    void postUpdateSources();

		void mixChannel(size_t channelNum, gsl::span<AudioSamplePack> dst);
    };
}
