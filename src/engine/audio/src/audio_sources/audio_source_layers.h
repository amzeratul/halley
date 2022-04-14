#pragma once
#include "audio_source.h"

namespace Halley
{
	class AudioObject;

	class AudioSourceLayers final : public AudioSource
	{
	public:
		AudioSourceLayers(AudioEngine& engine, std::shared_ptr<const AudioObject> object);

		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioSourceData& dst) override;
		bool isReady() const override;

	private:
		const std::shared_ptr<const AudioObject> object;
		std::unique_ptr<AudioSource> source;
	};
}
