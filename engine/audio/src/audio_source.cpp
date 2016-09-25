#include "audio_source.h"
#include "audio_mixer.h"

using namespace Halley;

AudioSource::AudioSource(std::shared_ptr<AudioClip> clip, AudioSourcePosition sourcePos, float gain, bool loop) 
	: clip(clip)
	, sourcePos(sourcePos)
	, looping(loop)
	, gain(gain)
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

size_t AudioSource::getNumberOfChannels() const
{
	return clip->getNumberOfChannels();
}

void AudioSource::update(gsl::span<const AudioChannelData> channels)
{
	Expects(playing);
	
	if (playbackPos == 0) {
		sourcePos.setMix(clip->getNumberOfChannels(), channels, channelMix, gain);
		prevChannelMix = channelMix;
	} else {
		prevChannelMix = channelMix;
		sourcePos.setMix(clip->getNumberOfChannels(), channels, channelMix, gain);
	}
}

void AudioSource::mixToBuffer(size_t srcChannel, size_t dstChannel, gsl::span<AudioSamplePack> tmp, gsl::span<AudioSamplePack> out, AudioMixer& mixer)
{
	Expects(playing);
	Expects(tmp.size() == out.size());
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

	size_t totalLen = size_t(tmp.size()) * 16; 
	if (canDoDirectRead(totalLen)) {
		auto src = clip->getChannelData(srcChannel, playbackPos, totalLen);
		mixer.mixAudio(gsl::span<const AudioSamplePack>(reinterpret_cast<const AudioSamplePack*>(src.data()), totalLen / 16), out, gain0, gain1);
	} else {
		readSourceToBuffer(srcChannel, tmp);
		mixer.mixAudio(tmp, out, gain0, gain1);
	}
}

void AudioSource::advancePlayback(size_t samples)
{
	playbackPos += samples;
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

bool AudioSource::canDoDirectRead(size_t size) const
{
	return playbackPos % 4 == 0 && playbackPos + size <= playbackLength;
}
