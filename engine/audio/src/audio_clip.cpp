#include "audio_clip.h"
#include "halley/resources/resource_data.h"
#include "vorbis_dec.h"

using namespace Halley;

AudioClip::AudioClip()
{
	startLoading();
}

void AudioClip::loadFromData(std::shared_ptr<ResourceDataStatic> data)
{
	VorbisData vorbis(data);
	size_t nChannels = vorbis.getChannels();
	if (vorbis.getFrequency() != AudioConfig::sampleRate) {
		throw Exception("Sound clip should be " + toString(AudioConfig::sampleRate) + " Hz.");
	}
	std::vector<char> rawData;
	vorbis.getData(rawData);
	vorbis.close();

	const AudioConfig::SampleFormat scale = 1.0f / 32768.0f;

	const short* src = reinterpret_cast<short*>(rawData.data());
	size_t nSamples = rawData.size() / (2 * nChannels);
	samples.resize(nChannels);
	for (size_t i = 0; i < nChannels; ++i) {
		samples[i].resize(nSamples);
	}

	if (nChannels == 1) {
		for (size_t i = 0; i < nSamples; ++i) {
			samples[0][i] = AudioConfig::SampleFormat(src[i]) * scale;
		}
	} else if (nChannels == 2) {
		for (size_t i = 0; i < nSamples; ++i) {
			samples[0][i] = AudioConfig::SampleFormat(src[i * 2]) * scale;
			samples[1][i] = AudioConfig::SampleFormat(src[i * 2 + 1]) * scale;
		}
	} else {
		throw Exception("Sound clip must have one or two channels.");
	}
	rawData.clear();

	sampleLength = nSamples;

	doneLoading();
}

void AudioClip::getChannelData(size_t channelN, size_t pos, gsl::span<AudioConfig::SampleFormat> dst) const
{
	Expects(isLoaded());
	Expects(pos < sampleLength);

	size_t requestedLen = size_t(dst.size());
	size_t len = std::min(requestedLen, sampleLength - pos);

	const AudioConfig::SampleFormat* src = samples.at(channelN).data() + pos;
	for (size_t i = 0; i < len; ++i) {
		dst[i] = src[i];
	}
	if (len != requestedLen) {
		for (size_t i = len; i < requestedLen; ++i) {
			dst[i] = AudioConfig::SampleFormat();
		}
	}
}

gsl::span<const AudioConfig::SampleFormat> AudioClip::getChannelData(size_t channelN, size_t pos, size_t len) const
{
	Expects(pos + len <= sampleLength);

	return gsl::span<const AudioConfig::SampleFormat>(samples.at(channelN).data() + pos, len);
}

size_t AudioClip::getLength() const
{
	Expects(isLoaded());
	return sampleLength;
}

size_t AudioClip::getNumberOfChannels() const
{
	Expects(isLoaded());
	return samples.size();
}

std::shared_ptr<AudioClip> AudioClip::loadResource(ResourceLoader& loader)
{
	auto result = std::make_shared<AudioClip>();

	loader
		.getAsync()
		.then([result](std::unique_ptr<ResourceDataStatic> data) {
			result->loadFromData(std::shared_ptr<ResourceDataStatic>(std::move(data)));
		});

	return result;
}
