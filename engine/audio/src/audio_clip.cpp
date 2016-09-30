#include "audio_clip.h"
#include "halley/resources/resource_data.h"
#include "vorbis_dec.h"
#include "halley/resources/metadata.h"
#include "halley/concurrency/concurrent.h"

using namespace Halley;

AudioClip::AudioClip(size_t numChannels)
	: numChannels(numChannels)
{
	startLoading();
}

AudioClip::~AudioClip()
{
}

void AudioClip::loadFromStatic(std::shared_ptr<ResourceDataStatic> data)
{
	VorbisData vorbis(data);
	size_t nChannels = vorbis.getChannels();
	if (vorbis.getFrequency() != AudioConfig::sampleRate) {
		throw Exception("Sound clip should be " + toString(AudioConfig::sampleRate) + " Hz.");
	}	
	size_t size = vorbis.getSize();
	size_t nSamples = size / nChannels;
	std::vector<short> src(size);
	vorbis.read(gsl::as_writeable_bytes(gsl::span<short>(src)));
	vorbis.close();

	const AudioConfig::SampleFormat scale = 1.0f / 32768.0f;

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
	src.clear();

	numChannels = nChannels;
	sampleLength = nSamples;
	streaming = false;
	doneLoading();
}

void AudioClip::loadFromStream(std::shared_ptr<ResourceDataStream> data)
{
	vorbisData = std::make_unique<VorbisData>(data);
	size_t nChannels = vorbisData->getChannels();
	if (vorbisData->getFrequency() != AudioConfig::sampleRate) {
		//throw Exception("Sound clip should be " + toString(AudioConfig::sampleRate) + " Hz.");
	}	
	size_t size = vorbisData->getSize();

	samples.resize(nChannels);
	numChannels = nChannels;
	sampleLength = size / nChannels;
	streaming = true;
	doneLoading();
}

gsl::span<const AudioConfig::SampleFormat> AudioClip::getChannelData(size_t channelN, size_t pos, size_t len)
{
	Expects(pos + len <= sampleLength);

	if (streaming) {
		auto& temp = pos == 0 ? temp0 : temp1; // pos == 0 has a different buffer because if it loops around, it will be used together with another buffer

		if (channelN == 0) { // TODO: this assumes the channels will be read in order. This will break threading.
			if (pos == 0) {
				vorbisData->seek(0);
			}

			size_t toRead = len * numChannels;
			if (temp.size() < toRead) {
				temp.resize(toRead);
			}
			vorbisData->read(gsl::as_writeable_bytes(gsl::span<short>(temp)));
		}

		auto& buf = samples.at(channelN);
		if (buf.size() < len) {
			buf.resize(len);
		}

		const AudioConfig::SampleFormat scale = 1.0f / 32768.0f;
		const short* src = reinterpret_cast<short*>(temp.data());

		for (size_t i = 0; i < len; ++i) {
			samples[channelN][i] = AudioConfig::SampleFormat(src[i * numChannels + channelN]) * scale;
		}
		return gsl::span<const AudioConfig::SampleFormat>(samples[channelN].data(), len);
	} else {
		return gsl::span<const AudioConfig::SampleFormat>(samples.at(channelN).data() + pos, len);
	}
}

size_t AudioClip::getLength() const
{
	Expects(isLoaded());
	return sampleLength;
}

size_t AudioClip::getNumberOfChannels() const
{
	return numChannels;
}

std::shared_ptr<AudioClip> AudioClip::loadResource(ResourceLoader& loader)
{
	bool streaming = loader.getMeta().getBool("streaming", false);
	int channels = loader.getMeta().getInt("channels", 1);
	auto result = std::make_shared<AudioClip>(size_t(channels));

	if (streaming) {
		std::shared_ptr<ResourceDataStream> stream = loader.getStream();
		Concurrent::execute([stream, result] ()	{
			result->loadFromStream(stream);
		});
	} else {
		loader
			.getAsync()
			.then([result](std::unique_ptr<ResourceDataStatic> data) {
				result->loadFromStatic(std::shared_ptr<ResourceDataStatic>(std::move(data)));
			});
	}

	return result;
}
