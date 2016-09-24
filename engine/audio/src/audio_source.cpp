#include "audio_source.h"

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

void AudioSource::readToBuffer(size_t srcChannel, size_t dstChannel, gsl::span<AudioConfig::SampleFormat> out)
{
	Expects(playing);
	Expects(srcChannel < 2);
	Expects(dstChannel < 8);

	size_t requestedLen = size_t(out.size());
	size_t len = std::min(requestedLen, playbackLength - playbackPos);
	auto src = clip->getChannelData(srcChannel, playbackPos, len);

	float gain0 = prevChannelMix[dstChannel];
	float gain1 = channelMix[dstChannel];
	float scale = 1.0f / out.size();

	if (gain0 == gain1) {
		// If the gain doesn't change, the code is faster
		for (size_t i = 0; i < len; ++i) {
			out[i] = src[i] * gain0;
		}
	} else {
		// Interpolate the gain
		for (size_t i = 0; i < len; ++i) {
			out[i] = src[i] * lerp(gain0, gain1, i * scale);
		}
	}
	for (size_t i = len; i < requestedLen; ++i) {
		out[i] = 0;
	}

	playbackPos += len;
	if (playbackPos >= playbackLength) {
		stop();
	}
}

void AudioSource::mixToBuffer(size_t srcChannel, size_t dstChannel, gsl::span<AudioConfig::SampleFormat> out)
{
	Expects(playing);
	Expects(srcChannel < 2);
	Expects(dstChannel < 8);

	size_t requestedLen = size_t(out.size());
	size_t len = std::min(requestedLen, playbackLength - playbackPos);
	auto src = clip->getChannelData(srcChannel, playbackPos, len);

	float gain0 = prevChannelMix[dstChannel];
	float gain1 = channelMix[dstChannel];
	float scale = 1.0f / out.size();

	if (gain0 == gain1) {
		// If the gain doesn't change, the code is faster
		for (size_t i = 0; i < len; ++i) {
			out[i] += src[i] * gain0;
		}
	} else {
		// Interpolate the gain
		for (size_t i = 0; i < len; ++i) {
			out[i] += src[i] * lerp(gain0, gain1, i * scale);
		}
	}

	playbackPos += len;
	if (playbackPos >= playbackLength) {
		stop();
	}
}
