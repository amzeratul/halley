#pragma once
#include "halley/audio/audio_source.h"

namespace Halley
{
	class AudioEmitter;

	class AudioSourceObject final : public AudioSource
	{
	public:
		AudioSourceObject(AudioEngine& engine, AudioEmitter& emitter, const AudioObject& object, bool loadSources);

		String getName() const override;

		bool isReady() const override;
		uint8_t getNumberOfChannels() const override;
		size_t getSamplesLeft() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		void restart() override;
		bool isLooping() override;
		void suspend() override;
		void resumeFromSuspension(AudioEngine& engine, AudioEmitter& emitter) override;

	private:
		const AudioObject& object;
		Vector<std::unique_ptr<AudioSource>> sources;
		bool looping = false;

		void loadSources(AudioEngine& engine, AudioEmitter& emitter);
		void unloadSources();
    };
}
