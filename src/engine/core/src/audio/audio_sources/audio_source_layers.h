#pragma once
#include "halley/audio/audio_fade.h"
#include "halley/audio/audio_source.h"
#include "halley/concurrency/alive_flag.h"

namespace Halley {
	class AudioFilterResample;
}

namespace Halley
{
	class AudioSourceDelay;
	class AudioSubObjectLayers;
	class AudioEmitter;
	class AudioObject;

	class AudioSourceLayers final : public AudioSource
	{
	public:
		AudioSourceLayers(AudioEngine& engine, AudioEmitter& emitter, const AudioSubObjectLayers& layerConfig);

		String getName() const override;
		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		bool isReady() const override;
		size_t getSamplesLeft() const override;
		void restart() override;
		bool isLooping() override;

	private:
		class Layer {
		public:
			float prevGain = 0;
			float gain = 0;
			bool playing = false;
			bool layerStarted = false;
			bool synchronised = false;
			size_t idx = 0;
			AudioFader fader;

			Layer(std::unique_ptr<AudioSource> source, size_t idx);

			void init(const AudioSubObjectLayers& layerConfig);
			void restart(const AudioSubObjectLayers& layerConfig, AudioEmitter& emitter, AudioEngine& engine);
			void update(float time, const AudioSubObjectLayers& layersConfig, AudioEmitter& emitter, AudioEngine& engine);

			void setSourceDelay(float delay);
			void setSourcePitch(float pitch, AudioEngine& engine);

			AudioSource& getSource() const;

		private:
			std::shared_ptr<AudioSource> origSource;
			std::shared_ptr<AudioSourceDelay> delaySource;
			std::shared_ptr<AudioFilterResample> resampleSource;
		};

		AudioEngine& engine;
		AudioEmitter& emitter;
		const AudioSubObjectLayers& layerConfig;
		Vector<Layer> layers;
		bool initialized = false;

		NonOwningAliveFlag layersAliveFlag;
		uint32_t version;

		void checkForReload();
		void setupLayers();
	};
}
