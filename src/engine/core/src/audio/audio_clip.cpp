#include "halley/audio/audio_clip.h"

#include "audio_mixer.h"
#include "halley/resources/resource_data.h"
#include "halley/audio/vorbis_dec.h"
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
	streaming = other.streaming;
	
	samples = std::move(other.samples);
	buffer = std::move(other.buffer);
	streamingResource = std::move(streamingResource);

	doneLoading();

	return *this;
}

void AudioClip::loadFromStatic(std::shared_ptr<ResourceDataStatic> data, Metadata metadata)
{
	load(std::move(data), std::move(metadata), false);
}

void AudioClip::loadFromStream(std::shared_ptr<ResourceDataStream> data, Metadata metadata)
{
	this->streamingResource = data;
	load(std::move(data), std::move(metadata), true);
}

void AudioClip::load(std::shared_ptr<ResourceData> data, Metadata metadata, bool stream)
{
	if (stream) {
		Logger::logInfo("Loading " + getAssetId());
	}

	auto vorbis = VorbisData(std::move(data), true);

	
	if (stream) {
		Logger::logInfo("Opened " + getAssetId());
	}

	if (vorbis.getSampleRate() != AudioConfig::sampleRate) {
		throw Exception("Sound clip should be " + toString(AudioConfig::sampleRate) + " Hz.", HalleyExceptions::AudioEngine);
	}	
	numChannels = static_cast<uint8_t>(vorbis.getNumChannels());
	sampleLength = vorbis.getNumSamples();
	loopPoint = metadata.getInt("loopPoint", 0);
	streaming = stream;

	samples.resize(numChannels);

	
	if (stream) {
		Logger::logInfo("Reading " + getAssetId());
	}

	if (!stream) {
		for (size_t i = 0; i < numChannels; ++i) {
			samples[i].resize(sampleLength);
		}
		vorbis.read(samples);
	}
	
	if (stream) {
		Logger::logInfo("Read " + getAssetId());
	}
	
	vorbis.close();

	doneLoading();
}

String AudioClip::getName() const
{
	return getAssetId();
}

bool AudioClip::hasStreamHandles() const
{
	return streaming;
}

std::unique_ptr<IAudioClipStreamHandle> AudioClip::makeStreamHandle() const
{
	return std::make_unique<AudioClipStreamHandle>(*this);
}

void AudioClip::prepareChannelData(size_t pos, size_t len, IAudioClipStreamHandle* streamHandle) const
{
	if (!streaming) {
		return;
	}

	if (!streamHandle) {
		Logger::logError("Stream handle not provided to AudioClip, but is a streaming clip", true);
		return;
	}

	try {
		Expects(pos + len <= sampleLength);

		if (buffer.size() != numChannels) {
			buffer.resize(numChannels);
		}
		for (auto& b: buffer) {
			if (b.size() <= len) {
				b.resize(len);
			}
		}

		auto& handle = *dynamic_cast<AudioClipStreamHandle*>(streamHandle);
		handle.seek(pos);
		handle.getVorbisData().read(buffer);
		handle.advance(len);
	} catch (std::exception& e) {
		Logger::logError("Exception while trying to read data from AudioClip \"" + getAssetId() + "\":");
		Logger::logException(e);
	} catch (...) {
		Logger::logError("Unknown exception while trying to read data from AudioClip \"" + getAssetId() + "\"");
	}
}

size_t AudioClip::copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const
{
	Expects(pos + len <= sampleLength);

	if (streaming) {
		AudioMixer::copy(dst, AudioSamples(buffer[channelN]).subspan(0, len), gain0, gain1);
	} else {
		AudioMixer::copy(dst, AudioSamples(samples.at(channelN)).subspan(pos, len), gain0, gain1);
	}
	return len;
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

ResourceMemoryUsage AudioClip::getMemoryUsage() const
{
	ResourceMemoryUsage result;

	for (auto& s: samples) {
		result.ramUsage += s.byte_span().size();
	}
	for (auto& b: buffer) {
		result.ramUsage += b.byte_span().size();
	}
	result.ramUsage += sizeof(*this);

	return result;
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

AudioClipStreamHandle::AudioClipStreamHandle(const AudioClip& parent)
{
	vorbisData = std::make_unique<VorbisData>(parent.streamingResource, true);
}

void AudioClipStreamHandle::seek(size_t pos)
{
	if (streamPos != pos) {
		vorbisData->seek(pos);
		streamPos = pos;
	}
}

void AudioClipStreamHandle::advance(size_t len)
{
	streamPos += len;
}

VorbisData& AudioClipStreamHandle::getVorbisData()
{
	return *vorbisData;
}
