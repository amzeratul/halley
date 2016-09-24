#pragma once
#include "halley/core/api/audio_api.h"
#include "audio_buffer.h"
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "audio_source.h"

namespace Halley {
    class AudioEngine : public AudioPlaybackAPI
    {
    public:
	    AudioEngine();

	    AudioCallback getCallback();
	    void playUI(std::shared_ptr<AudioClip> clip, float volume, float pan) override;

		void run();
		void start(AudioSpec spec);
		void stop();

    private:
		AudioSpec spec;

		std::atomic<bool> running;
		std::atomic<bool> needsBuffer;
		std::mutex mutex;
		std::condition_variable backBufferCondition;

		std::vector<std::unique_ptr<AudioSource>> sources;
		std::vector<AudioChannelData> channels;

		AudioBuffer backBuffer;
		AudioBuffer tmpBuffer;
    	std::vector<AudioBuffer> channelBuffers;

		void serviceAudio(gsl::span<AudioSamplePack> dst);
	    void generateBuffer();
		void updateSources();
	    void postUpdateSources();

		void mixChannel(size_t channelNum, gsl::span<AudioSamplePack> dst);
	    void interpolateChannels(AudioBuffer& dst, const std::vector<AudioBuffer>& src);
    };
}
