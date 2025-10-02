#include "audio_source_layers.h"

#include "halley/audio/audio_clip.h"
#include "../audio_engine.h"
#include "../audio_filter_resample.h"
#include "halley/audio/audio_object.h"
#include "audio_source_delay.h"
#include "../audio_mixer.h"
#include "halley/audio/sub_objects/audio_sub_object_layers.h"

using namespace Halley;

AudioSourceLayers::AudioSourceLayers(AudioEngine& engine, AudioEmitter& emitter, const AudioSubObjectLayers& layerConfig)
	: engine(engine)
	, emitter(emitter)
	, layerConfig(layerConfig)
	, layersAliveFlag(layerConfig.makeAliveFlag())
{
	setupLayers();
}

void AudioSourceLayers::setupLayers()
{
	layerConfig.validate(engine.getAudioProperties());

	auto layerSources = layerConfig.makeLayerSources(engine, emitter);

	layers.clear();
	layers.reserve(layerSources.size());
	for (size_t i = 0; i < layerSources.size(); ++i) {
		layers.emplace_back(std::move(layerSources[i]), i);
		layers.back().init(layerConfig);
	}

	initialized = false;
	version = layerConfig.getVersion();
}

void AudioSourceLayers::checkForReload()
{
	if (layerConfig.getVersion() != version) {
		setupLayers();
	}
}

String AudioSourceLayers::getName() const
{
	if (layersAliveFlag) {
		return layerConfig.getName();
	} else {
		return {};
	}
}

uint8_t AudioSourceLayers::getNumberOfChannels() const
{
	for (auto& layer: layers) {
		auto n = layer.getSource().getNumberOfChannels();
		if (n > 0) {
			return n;
		}
	}
	return 0;
}

bool AudioSourceLayers::getAudioData(size_t numSamples, AudioMultiChannelSamples dst)
{
	if (!layersAliveFlag) {
		return false;
	}

	checkForReload();

	for (const auto& layer: layers) {
		if (!layer.getSource().isReady()) {
			Logger::logError("AudioSourceLayers (" + getName() + ") error, layer " + toString(layer.idx) + " (" + layer.getSource().getName() + ") is not ready");
			return false;
		}
	}

	if (!initialized) {
		for (auto& layer : layers) {
			layer.restart(layerConfig, emitter, engine);
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
		layer.update(deltaTime, layerConfig, emitter, engine);
		if (layer.playing || layer.synchronised || layer.fader.isFading()) {
			ok = layer.getSource().getAudioData(numSamples, temp.getSampleSpans()) && ok;

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
	if (!layersAliveFlag) {
		return false;
	}
	return std::all_of(layers.begin(), layers.end(), [=] (const Layer& layer) {
		return layer.getSource().isReady();
	});
}

size_t AudioSourceLayers::getSamplesLeft() const
{
	size_t result = 0;
	for (auto& l: layers) {
		result = std::max(result, l.getSource().getSamplesLeft());
	}
	return result;
}

void AudioSourceLayers::restart()
{
	if (!layersAliveFlag) {
		return;
	}
	for (auto& layer: layers) {
		layer.restart(layerConfig, emitter, engine);
		layer.getSource().restart();
	}
}

bool AudioSourceLayers::isLooping()
{
	for (auto& layer: layers) {
		if (layer.getSource().isLooping()) {
			return true;
		}
	}
	return false;
}

AudioSourceLayers::Layer::Layer(std::unique_ptr<AudioSource> source, size_t idx)
	: idx(idx)
	, origSource(std::move(source))
{
}

void AudioSourceLayers::Layer::init(const AudioSubObjectLayers& layerConfig)
{
	const auto& curLayer = layerConfig.getLayer(idx);
	synchronised = curLayer.synchronised;
}

void AudioSourceLayers::Layer::restart(const AudioSubObjectLayers& layerConfig, AudioEmitter& emitter, AudioEngine& engine)
{
	const auto& curLayer = layerConfig.getLayer(idx);
	const auto targetGain = curLayer.gainExpression.evaluate(emitter, Range<float>(0, 1));
	const auto targetPitch = curLayer.pitchExpression.evaluate(emitter, Range(0.25f, 4.0f));
	fader.stopAndSetValue(targetGain);
	prevGain = gain = targetGain;
	playing = targetGain > 0.0001f;
	setSourcePitch(targetPitch, engine);
	setSourceDelay(playing ? 0 : curLayer.delay);
}

void AudioSourceLayers::Layer::update(float time, const AudioSubObjectLayers& layersConfig, AudioEmitter& emitter, AudioEngine& engine)
{
	const auto& generalFade = layersConfig.getFade();
	const auto& layer = layersConfig.getLayer(idx);
	const auto targetGain = layer.gainExpression.evaluate(emitter, Range<float>(0, 1));
	const auto targetPitch = layer.pitchExpression.evaluate(emitter, Range(0.25f, 4.0f));

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
	setSourcePitch(targetPitch, engine);

	prevGain = gain;
	gain = fader.getCurrentValue();

	const bool wasPlaying = playing;
	playing = gain > 0.0001f || prevGain > 0.0001f;

	if (playing) {
		layerStarted = true;
	} else if (wasPlaying && layer.restartFromBeginning) {
		setSourceDelay(layer.delay);
		getSource().restart();
		layerStarted = false;
	}
}

void AudioSourceLayers::Layer::setSourceDelay(float delay)
{
	const auto delaySamples = static_cast<size_t>(lroundl(delay * AudioConfig::sampleRate));

	if (delaySource) {
		delaySource->setInitialDelay(delaySamples);
	} else {
		auto curSource = resampleSource ? resampleSource : origSource;
		delaySource = std::make_shared<AudioSourceDelay>(curSource, delaySamples);
	}
}

void AudioSourceLayers::Layer::setSourcePitch(float pitch, AudioEngine& engine)
{
	const float toHz = 48000.0f;
	const float fromHz = toHz * pitch;

	if (resampleSource) {
		resampleSource->setFromHz(fromHz);
	} else if (std::abs(pitch - 1.0f) > 0.0001f) {
		resampleSource = std::make_shared<AudioFilterResample>(origSource, fromHz, toHz, engine.getPool());
		if (delaySource) {
			delaySource->setSource(resampleSource);
		}
	}
}

AudioSource& AudioSourceLayers::Layer::getSource() const
{
	if (delaySource) {
		return *delaySource;
	} else if (resampleSource) {
		return *resampleSource;
	} else {
		return *origSource;
	}
}
