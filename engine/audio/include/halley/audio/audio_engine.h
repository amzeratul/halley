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

	    AudioCallback getCallback();

    	void playUI(std::shared_ptr<AudioClip> clip, float volume, float pan) override;
	    void playWorld(std::shared_ptr<AudioClip> clip, Vector2f position, float volume) override;
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

		AudioBuffer frontBuffer;
		AudioBuffer backBuffer;
		AudioBuffer tmpBuffer;
    	std::vector<AudioBuffer> channelBuffers;

		void serviceAudio(gsl::span<AudioSamplePack> dst);
	    void generateBuffer();
		void updateSources();
	    void postUpdateSources();

		void mixChannel(size_t channelNum, gsl::span<AudioSamplePack> dst);
    };
}
