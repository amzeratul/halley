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
	if (vorbis.getSampleRate() != AudioConfig::sampleRate) {
		throw Exception("Sound clip should be " + toString(AudioConfig::sampleRate) + " Hz.");
	}	
	numChannels = vorbis.getNumChannels();
	sampleLength = vorbis.getNumSamples();
	streaming = false;

	samples.resize(numChannels);
	for (size_t i = 0; i < numChannels; ++i) {
		samples[i].resize(sampleLength);
	}
	vorbis.read(samples);
	vorbis.close();

	doneLoading();
}

void AudioClip::loadFromStream(std::shared_ptr<ResourceDataStream> data)
{
	vorbisData = std::make_unique<VorbisData>(data);
	size_t nChannels = vorbisData->getNumChannels();
	if (vorbisData->getSampleRate() != AudioConfig::sampleRate) {
		throw Exception("Sound clip should be " + toString(AudioConfig::sampleRate) + " Hz.");
	}
	
	samples.resize(nChannels);
	numChannels = nChannels;
	sampleLength = vorbisData->getNumSamples();
	streaming = true;
	doneLoading();
}

gsl::span<const AudioConfig::SampleFormat> AudioClip::getChannelData(size_t channelN, size_t pos, size_t len) const
{
	Expects(pos + len <= sampleLength);

	if (streaming) {
		auto& temp = pos == 0 ? temp0 : temp1; // pos == 0 has a different buffer because if it loops around, it will be used together with another buffer
		if (temp.size() != numChannels) {
			temp.resize(numChannels);
		}

		if (channelN == 0) { // TODO: this assumes the channels will be read in order. This will break threading.
			if (pos == 0) {
				vorbisData->seek(0);
			}

			size_t toRead = len;
			for (size_t i = 0; i < numChannels; ++i) {
				if (temp[i].size() < toRead) {
					temp[i].resize(toRead);
				}
			}
			vorbisData->read(temp);
		}

		auto& buf = samples.at(channelN);
		if (buf.size() < len) {
			buf.resize(len);
		}

		memcpy(samples[channelN].data(), temp[channelN].data(), len * sizeof(AudioConfig::SampleFormat));
		
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
