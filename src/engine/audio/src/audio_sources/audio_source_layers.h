#pragma once
#include "audio_fade.h"
#include "audio_source.h"

namespace Halley
{
	class AudioSubObjectLayers;
	class AudioEmitter;
	class AudioObject;

	class AudioSourceLayers final : public AudioSource
	{
	public:
		AudioSourceLayers(AudioEngine& engine, AudioEmitter& emitter, Vector<std::unique_ptr<AudioSource>> layerSources, const AudioSubObjectLayers& layerConfig, AudioFade fadeConfig);

		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioSourceData dst) override;
		bool isReady() const override;

	private:
		class Layer {
		public:
			std::unique_ptr<AudioSource> source;
			float prevGain = 0;
			float gain = 0;
			bool playing = false;
			bool synchronised = false;
			size_t idx = 0;
			AudioFader fader;

			Layer(std::unique_ptr<AudioSource> source, AudioEmitter& emitter, size_t idx);
			void init(const AudioSubObjectLayers& layerConfig, AudioEmitter& emitter);
			void update(float time, const AudioSubObjectLayers& layerConfig, AudioEmitter& emitter, const AudioFade& fade);
		};

		AudioEngine& engine;
		AudioEmitter& emitter;
		const AudioSubObjectLayers& layerConfig;
		Vector<Layer> layers;
		AudioFade fadeConfig;
	};
}
