#include "audio_source_layers.h"

#include "audio_clip.h"
#include "../audio_engine.h"
#include "../audio_filter_resample.h"
#include "audio_object.h"
#include "audio_source_clip.h"
#include "../audio_mixer.h"
#include "sub_objects/audio_sub_object_layers.h"

using namespace Halley;

AudioSourceLayers::AudioSourceLayers(AudioEngine& engine, AudioEmitter& emitter, Vector<std::unique_ptr<AudioSource>> layerSources, const AudioSubObjectLayers& layerConfig, AudioFade fadeConfig)
	: engine(engine)
	, emitter(emitter)
	, layerConfig(layerConfig)
	, fadeConfig(fadeConfig)
{
	layers.reserve(layerSources.size());
	for (size_t i = 0; i < layerSources.size(); ++i) {
		layers.emplace_back(std::move(layerSources[i]), emitter, i);
		layers.back().init(layerConfig, emitter);

		assert(layers[0].source->getNumberOfChannels() == layers[i].source->getNumberOfChannels());
	}
}

uint8_t AudioSourceLayers::getNumberOfChannels() const
{
	return layers.empty() ? 0 : layers[0].source->getNumberOfChannels();
}

bool AudioSourceLayers::getAudioData(size_t numSamples, AudioMultiChannelSamples dst)
{
	const auto nChannels = getNumberOfChannels();
	const float deltaTime = static_cast<float>(numSamples) / static_cast<float>(AudioConfig::sampleRate);

	auto temp = engine.getPool().getBuffers(nChannels, numSamples);
	auto result = engine.getPool().getBuffers(nChannels, numSamples);
	bool ok = true;

	AudioMixer::zero(result.getSpans(), nChannels);
	for (auto& layer: layers) {
		layer.update(deltaTime, layerConfig, emitter, fadeConfig);
		if (layer.playing || layer.synchronised) {
			ok = layer.source->getAudioData(numSamples, temp.getSampleSpans()) && ok;

			if (layer.playing) {
				AudioMixer::mixAudio(temp.getSpans(), result.getSpans(), layer.prevGain, layer.gain);
			}
		}
	}
	AudioMixer::copy(dst, result.getSpans(), nChannels);

	return ok;
}

bool AudioSourceLayers::isReady() const
{
	return std::all_of(layers.begin(), layers.end(), [=] (const auto& ls) { return ls.source->isReady(); });
}

AudioSourceLayers::Layer::Layer(std::unique_ptr<AudioSource> source, AudioEmitter& emitter, size_t idx)
	: source(std::move(source))
	, idx(idx)
{
}

void AudioSourceLayers::Layer::init(const AudioSubObjectLayers& layerConfig, AudioEmitter& emitter)
{
	const auto targetGain = layerConfig.getLayerExpression(idx).evaluate(emitter);
	fader.stopAndSetValue(targetGain);
	prevGain = gain = targetGain;
	synchronised = layerConfig.isLayerSynchronised(idx);
	playing = targetGain > 0.0001f;
}

void AudioSourceLayers::Layer::update(float time, const AudioSubObjectLayers& layerConfig, AudioEmitter& emitter, const AudioFade& fade)
{
	const auto targetGain = layerConfig.getLayerExpression(idx).evaluate(emitter);

	if (std::abs(targetGain - fader.getTargetValue()) > 0.001f) {
		fader.startFade(gain, targetGain, fade);
	}

	fader.update(time);

	prevGain = gain;
	gain = fader.getCurrentValue();

	playing = gain > 0.0001f || prevGain > 0.0001f;
}
