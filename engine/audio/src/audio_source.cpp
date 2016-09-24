#include "audio_source.h"
#include "audio_mixer.h"

using namespace Halley;

AudioSourcePosition::AudioSourcePosition(bool isUI, Vector3f pos)
	: isUI(isUI)
	, pos(pos)
{
}

AudioSourcePosition AudioSourcePosition::makeUI(float pan)
{
	return AudioSourcePosition(true, Vector3f(pan, 0, 0));
}

AudioSourcePosition AudioSourcePosition::makePositional(Vector3f pos)
{
	return AudioSourcePosition(false, pos);
}

void AudioSourcePosition::setMix(gsl::span<const AudioChannelData> channels, gsl::span<float, 8> dst, float gain) const
{
	// TODO
	dst[0] = 0.5f * gain;
	dst[1] = 0.5f * gain;
}

AudioSource::AudioSource(std::shared_ptr<AudioClip> clip, AudioSourcePosition sourcePos, float gain) 
	: clip(clip)
	, sourcePos(sourcePos)
	, gain(gain)
{}

void AudioSource::start()
{
	Expects(isReady());
	Expects(!playing);

	playing = true;
	playbackPos = 0;
	playbackLength = clip->getLength();
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
		sourcePos.setMix(channels, channelMix, gain);
		prevChannelMix = channelMix;
	} else {
		prevChannelMix = channelMix;
		sourcePos.setMix(channels, channelMix, gain);
	}
}

void AudioSource::mixToBuffer(size_t srcChannel, size_t dstChannel, gsl::span<AudioSamplePack> tmp, gsl::span<AudioSamplePack> out)
{
	Expects(playing);
	Expects(tmp.size() == out.size());
	Expects(dstChannel < 8);

	const float gain0 = prevChannelMix[dstChannel];
	const float gain1 = channelMix[dstChannel];
	if (gain0 < 0.01f && gain1 < 0.01f) {
		// Early out
		return;
	}

	readSourceToBuffer(srcChannel, tmp);

	AudioMixer::mixAudio(tmp, out, gain0, gain1);
}

void AudioSource::advancePlayback(size_t samples)
{
	playbackPos += samples;
	if (playbackPos >= playbackLength) {
		stop();
	}
}

void AudioSource::readSourceToBuffer(size_t srcChannel, gsl::span<AudioSamplePack> dst) const
{
	Expects(clip);
	Expects(srcChannel < 2);

	size_t requestedLen = size_t(dst.size()) * 16;
	size_t len = std::min(requestedLen, playbackLength - playbackPos);
	auto src = clip->getChannelData(srcChannel, playbackPos, len);

	memcpy(dst.data(), src.data(), src.size_bytes());
	if (src.size_bytes() != dst.size_bytes()) {
		memset(reinterpret_cast<char*>(dst.data()) + src.size_bytes(), 0, dst.size_bytes() - src.size_bytes());
	}
}
