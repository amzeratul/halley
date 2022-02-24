#include "audio_clip.h"
#include "halley/resources/resource_data.h"
#include "vorbis_dec.h"
#include "halley/resources/metadata.h"
#include "halley/concurrency/concurrent.h"
#include "halley/text/string_converter.h"

using namespace Halley;

AudioClip::AudioClip(uint8_t numChannels)
	: numChannels(numChannels)
{
	startLoading();
}

AudioClip::~AudioClip()
{
}

AudioClip& AudioClip::operator=(AudioClip&& other) noexcept
{
	other.waitForLoad(true);

	sampleLength = other.sampleLength;
	numChannels = other.numChannels;
	loopPoint = other.loopPoint;
	streamPos = other.streamPos;
	streaming = other.streaming;

	temp0 = std::move(other.temp0);
	temp1 = std::move(other.temp1);
	samples = std::move(other.samples);
	vorbisData = std::move(other.vorbisData);

	doneLoading();

	return *this;
}

void AudioClip::loadFromStatic(std::shared_ptr<ResourceDataStatic> data, Metadata metadata)
{
	VorbisData vorbis(data);
	if (vorbis.getSampleRate() != AudioConfig::sampleRate) {
		throw Exception("Sound clip should be " + toString(AudioConfig::sampleRate) + " Hz.", HalleyExceptions::AudioEngine);
	}	
	numChannels = vorbis.getNumChannels();
	sampleLength = vorbis.getNumSamples();
	loopPoint = metadata.getInt("loopPoint", 0);
	streaming = false;

	samples.resize(numChannels);
	for (size_t i = 0; i < numChannels; ++i) {
		samples[i].resize(sampleLength);
	}
	vorbis.read(samples);
	vorbis.close();

	doneLoading();
}

void AudioClip::loadFromStream(std::shared_ptr<ResourceDataStream> data, Metadata metadata)
{
	vorbisData = std::make_unique<VorbisData>(data);
	uint8_t nChannels = vorbisData->getNumChannels();
	if (vorbisData->getSampleRate() != AudioConfig::sampleRate) {
		throw Exception("Sound clip should be " + toString(AudioConfig::sampleRate) + " Hz.", HalleyExceptions::AudioEngine);
	}
	
	samples.resize(nChannels);
	numChannels = nChannels;
	sampleLength = vorbisData->getNumSamples();
	loopPoint = metadata.getInt("loopPoint", 0);
	streamPos = 0;
	streaming = true;
	doneLoading();
}

size_t AudioClip::copyChannelData(size_t channelN, size_t pos, size_t len, gsl::span<AudioConfig::SampleFormat> dst) const
{
	Expects(pos + len <= sampleLength);

	if (streaming) {
		auto& temp = pos == 0 ? temp0 : temp1; // pos == 0 has a different buffer because if it loops around, it will be used together with another buffer
		if (temp.size() != numChannels) {
			temp.resize(numChannels);
		}

		if (channelN == 0) { // TODO: this assumes the channels will be read in order. This will break threading.
			if (pos != streamPos) {
				vorbisData->seek(pos);
				streamPos = pos;
			}

			size_t toRead = len;
			for (size_t i = 0; i < numChannels; ++i) {
				if (temp[i].size() < toRead) {
					temp[i].resize(toRead);
				}
			}
			vorbisData->read(temp);
			streamPos += len;
		}

		auto& buf = samples.at(channelN);
		if (buf.size() < len) {
			buf.resize(len);
		}

		memcpy(dst.data(), temp[channelN].data(), len * sizeof(AudioConfig::SampleFormat));
		return len;
	} else {
		memcpy(dst.data(), samples.at(channelN).data() + pos, len * sizeof(AudioConfig::SampleFormat));
		return len;
	}
}

size_t AudioClip::getLength() const
{
	Expects(isLoaded());
	return sampleLength;
}

uint8_t AudioClip::getNumberOfChannels() const
{
	return numChannels;
}

size_t AudioClip::getLoopPoint() const
{
	Expects(isLoaded());
	return loopPoint;
}

bool AudioClip::isLoaded() const
{
	return AsyncResource::isLoaded();
}

std::shared_ptr<AudioClip> AudioClip::loadResource(ResourceLoader& loader)
{
	auto meta = loader.getMeta();
	bool streaming = meta.getBool("streaming", false);
	int channels = meta.getInt("channels", 1);
	auto result = std::make_shared<AudioClip>(uint8_t(channels));

	if (streaming) {
		std::shared_ptr<ResourceDataStream> stream = loader.getStream();
		Concurrent::execute([stream, result, meta] () {
			result->loadFromStream(stream, meta);
		});
	} else {
		loader
			.getAsync()
			.then([result, meta](std::unique_ptr<ResourceDataStatic> data) {
				result->loadFromStatic(std::shared_ptr<ResourceDataStatic>(std::move(data)), meta);
			});
	}

	return result;
}

void AudioClip::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<AudioClip&>(resource));
}

StreamingAudioClip::StreamingAudioClip(uint8_t numChannels)
	: numChannels(numChannels)
{
	buffers.resize(numChannels);
}

void StreamingAudioClip::addInterleavedSamples(gsl::span<const AudioConfig::SampleFormat> src)
{
	std::unique_lock<std::mutex> lock(mutex);

	const size_t nSamples = src.size() / numChannels;

	for (size_t i = 0; i < numChannels; ++i) {
		const size_t startSize = buffers[i].size();
		buffers[i].resize(startSize + nSamples);
		for (size_t j = 0; j < nSamples; ++j) {
			buffers[i][j + startSize] = src[i + j * numChannels];
		}
	}

	length += nSamples;
}

size_t StreamingAudioClip::copyChannelData(size_t channelN, size_t pos, size_t len, gsl::span<AudioConfig::SampleFormat> dst) const
{
	std::unique_lock<std::mutex> lock(mutex);

	auto& buffer = buffers[channelN];
	const size_t toWrite = std::min(len, buffer.size());

	memcpy(dst.data(), buffer.data(), toWrite * sizeof(AudioConfig::SampleFormat));
	buffer.erase(buffer.begin(), buffer.begin() + toWrite);

	if (toWrite < len) {
		memcpy(dst.data() + toWrite, buffer.data() + toWrite, (len - toWrite) * sizeof(AudioConfig::SampleFormat));
	}

	return len;
}

uint8_t StreamingAudioClip::getNumberOfChannels() const
{
	return numChannels;
}

size_t StreamingAudioClip::getLength() const
{
	return length;
}

size_t StreamingAudioClip::getSamplesLeft() const
{
	std::unique_lock<std::mutex> lock(mutex);
	return buffers.at(0).size();
}
