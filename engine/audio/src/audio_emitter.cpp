#include "audio_emitter.h"
#include "audio_mixer.h"
#include "audio_emitter_behaviour.h"
#include "audio_source.h"

using namespace Halley;

AudioEmitter::AudioEmitter(std::shared_ptr<AudioSource> source, AudioPosition sourcePos, float gain) 
	: source(source)
	, sourcePos(sourcePos)
	, gain(gain)
{}

AudioEmitter::~AudioEmitter()
{}

void AudioEmitter::setId(size_t i)
{
	id = i;
}

size_t AudioEmitter::getId() const
{
	return id;
}

void AudioEmitter::start()
{
	Expects(isReady());
	Expects(!playing);

	playing = true;
	nChannels = source->getNumberOfChannels();

	if (nChannels > 1) {
		sourcePos = AudioPosition::makeFixed();
	}
}

void AudioEmitter::stop()
{
	playing = false;
	done = true;
}

bool AudioEmitter::isPlaying() const
{
	return playing;
}

bool AudioEmitter::isReady() const
{
	return source->isReady();
}

bool AudioEmitter::isDone() const
{
	return done;
}

void AudioEmitter::setBehaviour(std::shared_ptr<AudioEmitterBehaviour> value)
{
	behaviour = std::move(value);
	elapsedTime = 0;
	behaviour->onAttach(*this);
}

void AudioEmitter::setGain(float g)
{
	gain = g;
}

void AudioEmitter::setAudioSourcePosition(AudioPosition s)
{
	if (nChannels == 1) {
		sourcePos = s;
	}
}

float AudioEmitter::getGain() const
{
	return gain;
}

size_t AudioEmitter::getNumberOfChannels() const
{
	return nChannels;
}

void AudioEmitter::update(gsl::span<const AudioChannelData> channels, const AudioListenerData& listener)
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
	sourcePos.setMix(nChannels, channels, channelMix, gain, listener);
	
	if (isFirstUpdate) {
		prevChannelMix = channelMix;
		isFirstUpdate = false;
	}
}

void AudioEmitter::mixTo(gsl::span<AudioBuffer> dst, AudioMixer& mixer, AudioBufferPool& pool)
{
	Expects(dst.size() > 0);
	Expects(this != nullptr);

	const size_t numPacks = dst[0].packs.size();
	const size_t numSamples = numPacks * 16;
	const size_t nSrcChannels = getNumberOfChannels();
	const size_t nDstChannels = size_t(dst.size());

	// Figure out the total mix in the previous update, and now. If it's zero, then there's nothing to listen here.
	float totalMix = 0.0f;
	const size_t nMixes = nSrcChannels * nDstChannels;
	Expects (nMixes < 16);
	for (size_t i = 0; i < nMixes; ++i) {
		totalMix += prevChannelMix[i] + channelMix[i];
	}
	if (totalMix < 0.01f) {
		// Early out, this sample is not audible
		return;
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
	bool playing = source->getAudioData(numSamples, audioSampleData);
	
	// Render each emitter channel
	for (size_t srcChannel = 0; srcChannel < nSrcChannels; ++srcChannel) {
		// Read to buffer
		for (size_t dstChannel = 0; dstChannel < nDstChannels; ++dstChannel) {
			// Compute mix
			const size_t mixIndex = (srcChannel * nChannels) + dstChannel;
			const float gain0 = prevChannelMix[mixIndex];
			const float gain1 = channelMix[mixIndex];

			// Render to destination
			if (gain0 + gain1 > 0.01f) {
				mixer.mixAudio(audioData[srcChannel], dst[dstChannel].packs, gain0, gain1);
			}
		}
	}

	advancePlayback(numSamples);
	if (!playing) {
		stop();
	}
}

void AudioEmitter::advancePlayback(size_t samples)
{
	elapsedTime += float(samples) / AudioConfig::sampleRate;
}
