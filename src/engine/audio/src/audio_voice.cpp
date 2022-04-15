#include "audio_voice.h"
#include <utility>

#include "audio_engine.h"
#include "audio_filter_resample.h"
#include "audio_mixer.h"
#include "behaviours/audio_voice_behaviour.h"
#include "audio_source.h"
#include "halley/support/logger.h"

using namespace Halley;

AudioVoice::AudioVoice(AudioEngine& engine, std::shared_ptr<AudioSource> src, float gain, float pitch, uint32_t delaySamples, uint8_t group) 
	: engine(engine)
	, group(group)
	, playing(false)
	, paused(false)
	, done(false)
	, isFirstUpdate(true)
	, baseGain(gain)
	, userGain(1.0f)
	, delaySamples(delaySamples)
	, source(std::move(src))
{
	fader.stopAndSetValue(1);
	setPitch(pitch);
}

AudioVoice::~AudioVoice() = default;

void AudioVoice::setIds(AudioEventId eventId, AudioObjectId audioObjectId)
{
	this->eventId = eventId;
	this->audioObjectId = audioObjectId;
}

AudioEventId AudioVoice::getEventId() const
{
	return eventId;
}

AudioObjectId AudioVoice::getAudioObjectId() const
{
	return audioObjectId;
}

void AudioVoice::start()
{
	Expects(isReady());
	Expects(!playing);

	playing = true;
	nChannels = source->getNumberOfChannels();
}

void AudioVoice::play(AudioFade fade)
{
	if (fade.hasFade()) {
		fader.startFade(0, 1, fade);
		fadeEnd = FadeEndBehaviour::None;
	}
}

void AudioVoice::stop(AudioFade fade)
{
	if (fade.hasFade()) {
		if (fadeEnd == FadeEndBehaviour::None) {
			// If we're fading to stop or pause, don't change the fade, just replace its behaviour
			fader.startFade(fader.getCurrentValue(), 0, fade);
		}
		fadeEnd = FadeEndBehaviour::Stop;
	} else {
		playing = false;
		done = true;
	}
}

void AudioVoice::pause(AudioFade fade)
{
	if (fade.hasFade()) {
		if (fadeEnd == FadeEndBehaviour::None) {
			// If we're fading to stop or pause, don't do anything
			fader.startFade(fader.getCurrentValue(), 0, fade);
			fadeEnd = FadeEndBehaviour::Pause;
		}
	} else {
		paused = true;
	}
}

void AudioVoice::resume(AudioFade fade)
{
	paused = false;
	if (fade.hasFade()) {
		if (!fader.isFading() || fadeEnd == FadeEndBehaviour::Pause) {
			// If we're already fading, only override a pause
			fader.startFade(fader.getCurrentValue(), 1, fade);
			fadeEnd = FadeEndBehaviour::None;
		}
	}
}

bool AudioVoice::isPlaying() const
{
	return playing;
}

bool AudioVoice::isReady() const
{
	return source->isReady();
}

bool AudioVoice::isDone() const
{
	return done;
}


void AudioVoice::addBehaviour(std::unique_ptr<AudioVoiceBehaviour> value)
{
	Expects(value);
	if (behaviour) {
		behaviour->addToChain(std::move(value));
	} else {
		behaviour = std::move(value);
	}
	elapsedTime = 0;
	behaviour->onAttach(*this);
}

uint8_t AudioVoice::getGroup() const
{
	return group;
}

void AudioVoice::setBaseGain(float gain)
{
	baseGain = gain;
}

float AudioVoice::getBaseGain() const
{
	return baseGain;
}

void AudioVoice::setUserGain(float gain)
{
	userGain = gain;
}

float AudioVoice::getUserGain() const
{
	return userGain;
}

float& AudioVoice::getDynamicGainRef()
{
	return dynamicGain;
}

void AudioVoice::setPitch(float pitch)
{
	if (resample || std::abs(pitch - 1.0f) > 0.01f) {
		const auto freq = static_cast<int>(lround(AudioConfig::sampleRate * pitch));
		
		if (resample) {
			resample->setFromHz(freq);
		} else {
			resample = std::make_shared<AudioFilterResample>(source, freq, AudioConfig::sampleRate, engine.getPool());
			source = resample;
		}
	}
}

size_t AudioVoice::getNumberOfChannels() const
{
	return nChannels;
}

void AudioVoice::update(gsl::span<const AudioChannelData> channels, const AudioPosition& sourcePos, const AudioListenerData& listener, float groupGain)
{
	Expects(playing);

	if (fader.isFading()) {
		if (fader.update(elapsedTime)) {
			onFadeEnd();
		}
	}

	// Dynamic gain is a combination of the fader's current value, times the value from the current behaviour
	dynamicGain = fader.getCurrentValue();
	
	if (behaviour) {
		const bool keep = behaviour->updateChain(elapsedTime, *this);
		if (!keep) {
			behaviour = behaviour->releaseNext();
		}
	}
	elapsedTime = 0;

	const float pauseGain = paused ? 0.0f : 1.0f;
	
	prevChannelMix = channelMix;
	sourcePos.setMix(nChannels, channels, channelMix, baseGain * userGain * dynamicGain * groupGain * pauseGain, listener);
	
	if (isFirstUpdate) {
		prevChannelMix = channelMix;
		isFirstUpdate = false;
	}
}

void AudioVoice::onFadeEnd()
{
	switch (fadeEnd) {
	case FadeEndBehaviour::Pause:
		pause(AudioFade());
		break;
	case FadeEndBehaviour::Stop:
		stop(AudioFade());
		break;
	}
}


void AudioVoice::mixTo(size_t numSamplesRequested, gsl::span<AudioBuffer*> dst, AudioBufferPool& pool)
{
	Expects(!dst.empty());

	if (paused) {
		return;
	}

	// Figure out the total mix in the previous update, and now. If it's zero, then there's nothing to listen here.
	const size_t nSrcChannels = getNumberOfChannels();
	const auto nDstChannels = size_t(dst.size());
	float totalMix = 0.0f;
	const size_t nMixes = nSrcChannels * nDstChannels;
	Expects (nMixes < 16);
	for (size_t i = 0; i < nMixes; ++i) {
		totalMix += prevChannelMix[i] + channelMix[i];
	}

	// Check delay
	size_t startDstSample = 0;
	size_t numSamples = numSamplesRequested;
	if (delaySamples > 0) {
		const size_t delayNow = std::min(static_cast<size_t>(delaySamples), numSamples);
		delaySamples -= static_cast<uint32_t>(delayNow);
		startDstSample += delayNow;
		numSamples -= delayNow;
	}

	if (numSamples > 0) {
		// Read data from source
		AudioMultiChannelSamples audioData;
		AudioMultiChannelSamples audioSampleData;
		std::array<AudioBufferRef, AudioConfig::maxChannels> bufferRefs;
		for (size_t srcChannel = 0; srcChannel < nSrcChannels; ++srcChannel) {
			bufferRefs[srcChannel] = pool.getBuffer(numSamples);
			audioData[srcChannel] = bufferRefs[srcChannel].getSpan().subspan(0, numSamples);
			audioSampleData[srcChannel] = audioData[srcChannel];
		}
		const bool isPlaying = source->getAudioData(numSamples, audioSampleData);

		// If we're audible, render
		if (totalMix >= 0.0001f) {
			// Render each emitter channel
			for (size_t srcChannel = 0; srcChannel < nSrcChannels; ++srcChannel) {
				// Read to buffer
				for (size_t dstChannel = 0; dstChannel < nDstChannels; ++dstChannel) {
					// Compute mix
					const size_t mixIndex = (srcChannel * nChannels) + dstChannel;
					const float gain0 = prevChannelMix[mixIndex];
					const float gain1 = channelMix[mixIndex];

					// Render to destination
					if (gain0 + gain1 > 0.0001f) {
						const auto dstBuffer = AudioSamples(dst[dstChannel]->samples).subspan(startDstSample);
						AudioMixer::mixAudio(audioData[srcChannel], dstBuffer, gain0, gain1);
					}
				}
			}
		}

		advancePlayback(numSamples);

		if (!isPlaying) {
			stop(AudioFade());
		}
	}
}

void AudioVoice::advancePlayback(size_t samples)
{
	if (!paused) {
		elapsedTime += float(samples) / AudioConfig::sampleRate;
	}
}
