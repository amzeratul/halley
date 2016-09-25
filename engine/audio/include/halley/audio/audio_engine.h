#pragma once
#include "halley/core/api/audio_api.h"
#include "audio_buffer.h"
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "audio_source.h"

namespace Halley {
	class AudioMixer;

    class AudioEngine : public AudioPlaybackAPI
    {
    public:
	    AudioEngine();
		~AudioEngine();

    	void playUI(std::shared_ptr<AudioClip> clip, float volume, float pan, bool loop) override;
	    void playWorld(std::shared_ptr<AudioClip> clip, Vector2f position, float volume, bool loop) override;
	    void setListener(Vector2f position) override;

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
