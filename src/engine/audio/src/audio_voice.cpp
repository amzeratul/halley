#include "audio_voice.h"
#include <utility>
#include "audio_mixer.h"
#include "audio_voice_behaviour.h"
#include "audio_source.h"
#include "halley/support/logger.h"

using namespace Halley;

AudioVoice::AudioVoice(std::shared_ptr<AudioSource> source, AudioPosition sourcePos, float gain, uint8_t group) 
	: group(group)
	, gain(gain)
	, source(std::move(source))
	, sourcePos(std::move(sourcePos))
{}

AudioVoice::~AudioVoice() = default;

void AudioVoice::setId(uint32_t i)
{
	id = i;
}

uint32_t AudioVoice::getId() const
{
	return id;
}

void AudioVoice::start()
{
	Expects(isReady());
	Expects(!playing);

	playing = true;
	nChannels = source->getNumberOfChannels();
}

void AudioVoice::stop()
{
	playing = false;
	done = true;
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

void AudioVoice::setBehaviour(std::shared_ptr<AudioVoiceBehaviour> value)
{
	behaviour = std::move(value);
	elapsedTime = 0;
	behaviour->onAttach(*this);
}

uint8_t AudioVoice::getGroup() const
{
	return group;
}

void AudioVoice::setGain(float g)
{
	gain = g;
}

void AudioVoice::setAudioSourcePosition(Vector3f position)
{
	sourcePos.setPosition(position);
}

void AudioVoice::setAudioSourcePosition(AudioPosition s)
{
	sourcePos = std::move(s);
}

float AudioVoice::getGain() const
{
	return gain;
}

size_t AudioVoice::getNumberOfChannels() const
{
	return nChannels;
}

void AudioVoice::update(gsl::span<const AudioChannelData> channels, const AudioListenerData& listener, float groupGain)
{
	Expects(playing);

	if (behaviour) {
		bool keep = behaviour->update(elapsedTime, *this);
		if (!keep) {
			behaviour.reset();
		}
		elapsedTime = 0;
	}

	prevChannelMix = channelMix;
	sourcePos.setMix(nChannels, channels, channelMix, gain * groupGain, listener);
	
	if (isFirstUpdate) {
		prevChannelMix = channelMix;
		isFirstUpdate = false;
	}
}

void AudioVoice::mixTo(size_t numSamples, gsl::span<AudioBuffer*> dst, AudioMixer& mixer, AudioBufferPool& pool)
{
	Expects(dst.size() > 0);
	Expects(numSamples % 16 == 0);

	const size_t numPacks = numSamples / 16;
	Expects(dst[0]->packs.size() >= numPacks);
	const size_t nSrcChannels = getNumberOfChannels();
	const auto nDstChannels = size_t(dst.size());

	// Figure out the total mix in the previous update, and now. If it's zero, then there's nothing to listen here.
	float totalMix = 0.0f;
	const size_t nMixes = nSrcChannels * nDstChannels;
	Expects (nMixes < 16);
	for (size_t i = 0; i < nMixes; ++i) {
		totalMix += prevChannelMix[i] + channelMix[i];
	}

	// Read data from source
	std::array<gsl::span<AudioSamplePack>, AudioConfig::maxChannels> audioData;
	std::array<gsl::span<AudioConfig::SampleFormat>, AudioConfig::maxChannels> audioSampleData;
	std::array<AudioBufferRef, AudioConfig::maxChannels> bufferRefs;
	for (size_t srcChannel = 0; srcChannel < nSrcChannels; ++srcChannel) {
		bufferRefs[srcChannel] = pool.getBuffer(numSamples);
		audioData[srcChannel] = bufferRefs[srcChannel].getSpan().subspan(0, numPacks);
		audioSampleData[srcChannel] = audioData[srcChannel].data()->samples;
	}
	bool isPlaying = source->getAudioData(numSamples, audioSampleData);

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
					mixer.mixAudio(audioData[srcChannel], dst[dstChannel]->packs, gain0, gain1);
				}
			}
		}
	}

	advancePlayback(numSamples);
	if (!isPlaying) {
		stop();
	}
}

void AudioVoice::advancePlayback(size_t samples)
{
	elapsedTime += float(samples) / AudioConfig::sampleRate;
}
