#pragma once
#include "audio_source.h"

namespace Halley
{
	class AudioEmitter;
	class AudioObject;

	class AudioSourceLayers final : public AudioSource
	{
	public:
		AudioSourceLayers(AudioEngine& engine, AudioEmitter& emitter, Vector<std::unique_ptr<AudioSource>> layerSources);

		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioSourceData dst) override;
		bool isReady() const override;

	private:
		class Layer {
		public:
			std::unique_ptr<AudioSource> source;
			float prevGain = 0;
			float gain = 0;

			Layer(std::unique_ptr<AudioSource> source, AudioEmitter& emitter);
			void evaluateGain(AudioEmitter& emitter);
		};

		AudioEngine& engine;
		AudioEmitter& emitter;
		Vector<Layer> layers;
	};
}
