#include "audio_source_layers.h"

#include "halley/audio/audio_clip.h"
#include "../audio_engine.h"
#include "../audio_filter_resample.h"
#include "halley/audio/audio_object.h"
#include "audio_source_clip.h"
#include "audio_source_delay.h"
#include "../audio_mixer.h"
#include "halley/audio/sub_objects/audio_sub_object_layers.h"

using namespace Halley;

AudioSourceLayers::AudioSourceLayers(AudioEngine& engine, AudioEmitter& emitter, Vector<std::unique_ptr<AudioSource>> layerSources, const AudioSubObjectLayers& layerConfig, AudioFade fadeConfig)
	: engine(engine)
	, emitter(emitter)
	, layerConfig(layerConfig)
	, fadeConfig(fadeConfig)
{
	layers.reserve(layerSources.size());
	for (size_t i = 0; i < layerSources.size(); ++i) {
		layers.emplace_back(std::move(layerSources[i]), i);
		layers.back().init(layerConfig);
	}
}

uint8_t AudioSourceLayers::getNumberOfChannels() const
{
	for (auto& layer: layers) {
		auto n = layer.source->getNumberOfChannels();
		if (n > 0) {
			return n;
		}
	}
	return 0;
}

bool AudioSourceLayers::getAudioData(size_t numSamples, AudioMultiChannelSamples dst)
{
	if (!initialized) {
		for (auto& layer : layers) {
			layer.restart(layerConfig, emitter);
		}
		initialized = true;
	}

	const auto nChannels = getNumberOfChannels();
	const float deltaTime = static_cast<float>(numSamples) / static_cast<float>(AudioConfig::sampleRate);

	auto temp = engine.getPool().getBuffers(nChannels, numSamples);
	auto result = engine.getPool().getBuffers(nChannels, numSamples);
	bool ok = true;

	AudioMixer::zero(result.getSpans(), nChannels);
	for (auto& layer: layers) {
		layer.update(deltaTime, layerConfig, emitter, fadeConfig);
		if (layer.playing || layer.synchronised || layer.fader.isFading()) {
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

size_t AudioSourceLayers::getSamplesLeft() const
{
	size_t result = 0;
	for (auto& l: layers) {
		result = std::max(result, l.source->getSamplesLeft());
	}
	return result;
}

void AudioSourceLayers::restart()
{
	for (auto& layer: layers) {
		layer.restart(layerConfig, emitter);
		layer.source->restart();
	}
}

AudioSourceLayers::Layer::Layer(std::unique_ptr<AudioSource> source, size_t idx)
	: source(std::move(source))
	, idx(idx)
{
}

void AudioSourceLayers::Layer::init(const AudioSubObjectLayers& layerConfig)
{
	const auto& curLayer = layerConfig.getLayer(idx);
	synchronised = curLayer.synchronised;
}

void AudioSourceLayers::Layer::restart(const AudioSubObjectLayers& layerConfig, AudioEmitter& emitter)
{
	const auto& curLayer = layerConfig.getLayer(idx);
	const auto targetGain = curLayer.expression.evaluate(emitter);
	fader.stopAndSetValue(targetGain);
	prevGain = gain = targetGain;
	playing = targetGain > 0.0001f;
	setSourceDelay(playing ? 0 : curLayer.delay);
}

void AudioSourceLayers::Layer::setSourceDelay(float delay)
{
	const auto delaySamples = static_cast<size_t>(lroundl(delay * AudioConfig::sampleRate));
	if (auto* delaySource = dynamic_cast<AudioSourceDelay*>(source.get())) {
		delaySource->setInitialDelay(delaySamples);
	} else if (delay > 0.0001f) {
		source = std::make_unique<AudioSourceDelay>(std::move(source), delaySamples);
	}
}

void AudioSourceLayers::Layer::update(float time, const AudioSubObjectLayers& layersConfig, AudioEmitter& emitter, const AudioFade& generalFade)
{
	const auto& layer = layersConfig.getLayer(idx);
	const auto targetGain = layer.expression.evaluate(emitter);

	const auto delta = targetGain - fader.getTargetValue();
	if (std::abs(delta) > 0.001f) {
		const bool fadingIn = delta > 0;
		const bool fromScratch = !playing && !layerStarted;

		if (fadingIn && layer.onlyFadeInWhenResuming && fromScratch) {
			fader.stopAndSetValue(targetGain);
		} else {
			const auto& fade = fadingIn ? layer.fadeIn.value_or(generalFade) : layer.fadeOut.value_or(generalFade);
			fader.startFade(gain, targetGain, fade);
		}
	}

	fader.update(time);

	prevGain = gain;
	gain = fader.getCurrentValue();

	const bool wasPlaying = playing;
	playing = gain > 0.0001f || prevGain > 0.0001f;

	if (playing) {
		layerStarted = true;
	} else if (wasPlaying && layer.restartFromBeginning) {
		setSourceDelay(layer.delay);
		source->restart();
		layerStarted = false;
	}
}
