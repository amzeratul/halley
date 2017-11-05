#include "audio_emitter.h"
#include "audio_mixer.h"
#include "audio_emitter_behaviour.h"

using namespace Halley;

AudioEmitter::AudioEmitter(std::shared_ptr<const AudioClip> clip, AudioPosition sourcePos, float gain, bool loop) 
	: clip(clip)
	, sourcePos(sourcePos)
	, looping(loop)
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
	playbackPos = 0;
	playbackLength = clip->getLength();
	nChannels = clip->getNumberOfChannels();

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
	return clip->isLoaded();
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
	
	// Render each emitter channel
	auto tmp = pool.getBuffer(numSamples);
	for (size_t srcChannel = 0; srcChannel < nSrcChannels; ++srcChannel) {
		// Read to buffer
		readSourceToBuffer(srcChannel, tmp.getSpan().subspan(0, numPacks));

		for (size_t dstChannel = 0; dstChannel < nDstChannels; ++dstChannel) {
			// Compute mix
			const size_t mixIndex = (srcChannel * nChannels) + dstChannel;
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

void AudioEmitter::readSourceToBuffer(size_t srcChannel, gsl::span<AudioSamplePack> dst) const
{
	Expects(clip);
	Expects(srcChannel < 2);
	Expects(playbackPos <= playbackLength);

	const size_t requestedLen = size_t(dst.size()) * 16;
	const size_t len = std::min(requestedLen, playbackLength - playbackPos);
	const size_t remainingLen = requestedLen - len;

	if (len > 0) {
		auto src = clip->getChannelData(srcChannel, playbackPos, len);
		Expects(size_t(src.size_bytes()) <= requestedLen * sizeof(AudioConfig::SampleFormat));
		if (src.size_bytes() > 0) {
			memcpy(dst.data(), src.data(), src.size_bytes());
		}
	}

	if (remainingLen > 0) {
		const size_t remainingBytes = remainingLen * sizeof(AudioConfig::SampleFormat);
		char* dst2 = reinterpret_cast<char*>(dst.data()) + len * sizeof(AudioConfig::SampleFormat);
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

void AudioEmitter::advancePlayback(size_t samples)
{
	elapsedTime += float(samples) / AudioConfig::sampleRate;

	playbackPos += samples;
	if (playbackPos >= playbackLength) {
		if (looping) {
			playbackPos %= playbackLength;
		} else {
			playbackPos = playbackLength;
			stop();
		}
	}
}
