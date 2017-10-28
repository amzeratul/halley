#include "audio_source.h"
#include "audio_mixer.h"
#include "audio_source_behaviour.h"

using namespace Halley;

AudioSource::AudioSource(std::shared_ptr<const AudioClip> clip, AudioSourcePosition sourcePos, float gain, bool loop) 
	: clip(clip)
	, sourcePos(sourcePos)
	, looping(loop)
	, gain(gain)
{}

AudioSource::~AudioSource()
{}

void AudioSource::setId(size_t i)
{
	id = i;
}

size_t AudioSource::getId() const
{
	return id;
}

void AudioSource::start()
{
	Expects(isReady());
	Expects(!playing);

	playing = true;
	playbackPos = 0;
	playbackLength = clip->getLength();

	if (clip->getNumberOfChannels() > 1) {
		sourcePos = AudioSourcePosition::makeFixed();
	}
}

void AudioSource::stop()
{
	playing = false;
	done = true;
}

bool AudioSource::isPlaying() const
{
	return playing;
}

bool AudioSource::isReady() const
{
	return clip->isLoaded();
}

bool AudioSource::isDone() const
{
	return done;
}

void AudioSource::setBehaviour(std::shared_ptr<AudioSourceBehaviour> value)
{
	behaviour = std::move(value);
	elapsedTime = 0;
	behaviour->onAttach(*this);
}

void AudioSource::setGain(float g)
{
	gain = g;
}

void AudioSource::setAudioSourcePosition(AudioSourcePosition s)
{
	if (clip->getNumberOfChannels() == 1) {
		sourcePos = s;
	}
}

float AudioSource::getGain() const
{
	return gain;
}

size_t AudioSource::getNumberOfChannels() const
{
	return clip->getNumberOfChannels();
}

void AudioSource::update(gsl::span<const AudioChannelData> channels, const AudioListenerData& listener)
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
	sourcePos.setMix(clip->getNumberOfChannels(), channels, channelMix, gain, listener);
	
	if (isFirstUpdate) {
		prevChannelMix = channelMix;
		isFirstUpdate = false;
	}
}

void AudioSource::mixTo(gsl::span<AudioBuffer> dst, AudioMixer& mixer, AudioBufferPool& pool)
{
	Expects(playing);
	Expects(dst.size() > 0);

	const size_t numPacks = dst[0].packs.size();
	const size_t numSamples = numPacks * 16;
	const size_t nSrcChannels = getNumberOfChannels();
	const size_t nDstChannels = size_t(dst.size());

	// Figure out the total mix in the previous update, and now. If it's zero, then there's nothing to listen here.
	float totalMix = 0.0f;
	const size_t nMixes = nSrcChannels * nDstChannels;
	for (size_t i = 0; i < nMixes; ++i) {
		totalMix += prevChannelMix[i] + channelMix[i];
	}
	if (totalMix < 0.01f) {
		// Early out, this sample is not audible
		return;
	}
	
	// Render each source channel
	for (size_t srcChannel = 0; srcChannel < nSrcChannels; ++srcChannel) {
		// Read to buffer
		auto tmp = pool.getBuffer(numSamples);
		readSourceToBuffer(srcChannel, tmp.getSpan().subspan(0, numPacks));

		for (size_t dstChannel = 0; dstChannel < nDstChannels; ++dstChannel) {
			// Compute mix
			const size_t mixIndex = (srcChannel * clip->getNumberOfChannels()) + dstChannel;
			const float gain0 = prevChannelMix[mixIndex];
			const float gain1 = channelMix[mixIndex];

			// Render to destination
			if (gain0 + gain1 > 0.01f) {
				mixer.mixAudio(tmp.getSpan(), dst[dstChannel].packs, gain0, gain1);
			}
		}
	}

	advancePlayback(numSamples);
}

void AudioSource::readSourceToBuffer(size_t srcChannel, gsl::span<AudioSamplePack> dst) const
{
	Expects(clip);
	Expects(srcChannel < 2);

	size_t requestedLen = size_t(dst.size()) * 16;
	size_t len = std::min(requestedLen, playbackLength - playbackPos);
	size_t remainingLen = requestedLen - len;
	auto src = clip->getChannelData(srcChannel, playbackPos, len);

	memcpy(dst.data(), src.data(), src.size_bytes());

	if (remainingLen > 0) {
		size_t remainingBytes = remainingLen * sizeof(AudioConfig::SampleFormat);
		char* dst2 = reinterpret_cast<char*>(dst.data()) + src.size_bytes();
		if (looping) {
			// Copy from start
			auto src2 = clip->getChannelData(srcChannel, 0, len);
			memcpy(dst2, src2.data(), remainingBytes);
		} else {
			// Pad with zeroes
			memset(dst2, 0, remainingBytes);
		}
	}
}

void AudioSource::advancePlayback(size_t samples)
{
	elapsedTime += float(samples) / AudioConfig::sampleRate;

	playbackPos += samples;
	if (playbackPos >= playbackLength) {
		if (looping) {
			playbackPos %= playbackLength;
		} else {
			stop();
		}
	}
}
