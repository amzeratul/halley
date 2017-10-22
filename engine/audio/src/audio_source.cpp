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

void AudioSource::mixToBuffer(size_t srcChannel, size_t dstChannel, gsl::span<AudioSamplePack> out, AudioMixer& mixer, AudioBufferPool& pool)
{
	Expects(playing);
	Expects(dstChannel < 8);

	const size_t numChannels = clip->getNumberOfChannels();
	Expects(srcChannel < numChannels);

	const size_t mixIndex = (srcChannel * numChannels) + dstChannel;
	const float gain0 = prevChannelMix[mixIndex];
	const float gain1 = channelMix[mixIndex];
	if (gain0 < 0.01f && gain1 < 0.01f) {
		// Early out
		return;
	}

	size_t totalLen = size_t(out.size()) * 16; 
	if (canDoDirectRead(totalLen)) {
		auto src = clip->getChannelData(srcChannel, playbackPos, totalLen);
		mixer.mixAudio(gsl::span<const AudioSamplePack>(reinterpret_cast<const AudioSamplePack*>(src.data()), totalLen / 16), out, gain0, gain1);
	} else {
		auto tmp = pool.getBuffer(totalLen);
		readSourceToBuffer(srcChannel, tmp.getSpan());
		mixer.mixAudio(tmp.getSpan(), out, gain0, gain1);
	}
}

bool AudioSource::canDoDirectRead(size_t size) const
{
	return playbackPos % 4 == 0 && playbackPos + size <= playbackLength;
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
	playbackPos += samples;
	elapsedTime += float(samples) / AudioConfig::sampleRate;
	if (playbackPos >= playbackLength) {
		if (looping) {
			playbackPos %= playbackLength;
		} else {
			stop();
		}
	}
}

void AudioSource::setId(size_t i)
{
	id = i;
}

size_t AudioSource::getId() const
{
	return id;
}

void AudioSource::setBehaviour(std::shared_ptr<AudioSourceBehaviour> value)
{
	behaviour = std::move(value);
	elapsedTime = 0;
	behaviour->onAttach(*this);
}

