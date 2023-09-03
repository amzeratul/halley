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
	streamPos = other.streamPos;
	streaming = other.streaming;
	
	samples = std::move(other.samples);
	vorbisData = std::move(other.vorbisData);

	doneLoading();

	return *this;
}

void AudioClip::loadFromStatic(std::shared_ptr<ResourceDataStatic> data, Metadata metadata)
{
	VorbisData vorbis(data, true);
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
	for (size_t i = 0; i < vorbisData.size(); ++i) {
		vorbisData[i] = std::make_unique<VorbisData>(data, i == 0);
	}

	uint8_t nChannels = vorbisData[0]->getNumChannels();
	if (vorbisData[0]->getSampleRate() != AudioConfig::sampleRate) {
		throw Exception("Sound clip should be " + toString(AudioConfig::sampleRate) + " Hz.", HalleyExceptions::AudioEngine);
	}
	
	samples.resize(nChannels);
	numChannels = nChannels;
	sampleLength = vorbisData[0]->getNumSamples();
	loopPoint = metadata.getInt("loopPoint", 0);
	streamPos = 0;
	streaming = true;
	doneLoading();
}

size_t AudioClip::copyChannelData(size_t channelN, size_t pos, size_t len, float gain0, float gain1, AudioSamples dst) const
{
	Expects(pos + len <= sampleLength);

	if (streaming) {
		// NB: this assumes the channels will be read in order.
		if (channelN == 0) {
			if (buffer.size() != numChannels) {
				buffer.resize(numChannels);
			}
			for (auto& b: buffer) {
				if (b.size() <= len) {
					b.resize(len);
				}
			}

			auto* vorbis = getVorbisData(pos);
			if (vorbis) {
				vorbis->read(buffer);
			} else {
				for (auto& b : buffer) {
					AudioMixer::zero(b);
				}
			}
			streamPos = pos + len;
		}

		AudioMixer::copy(dst, AudioSamples(buffer[channelN]).subspan(0, len), gain0, gain1);
	} else {
		AudioMixer::copy(dst, AudioSamples(samples.at(channelN)).subspan(pos, len), gain0, gain1);
	}
	return len;
}

VorbisData* AudioClip::getVorbisData(size_t targetPos) const
{
	// This chooses which of the two vorbis data readers to use. This allows two simultaneous reads of the stream without insane seeking, needed for self-overlapping music loops.
	// It'll basically pick whichever of the two readers is closer to the target position, and seek if needed.

	const size_t nSamples = vorbisData[0]->getNumSamples();
	if (nSamples == 0) {
		// Happens when resource is unloaded, e.g. due to hot reload
		return nullptr;
	}
	size_t bestDist = std::numeric_limits<size_t>::max();
	size_t bestIdx = 0;

	for (size_t i = 0; i < vorbisData.size(); ++i) {
		const auto curPos = vorbisData[i]->tell();

		const auto dist = (nSamples + targetPos - curPos) % nSamples;

		if (dist < bestDist) {
			bestDist = dist;
			bestIdx = i;
		}
	}

	if (bestDist != 0) {
		vorbisData[bestIdx]->seek(targetPos);
	}
	
	return vorbisData[bestIdx].get();
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

	for (auto& v: vorbisData) {
		if (v) {
			result.ramUsage += v->getSizeBytes() + sizeof(VorbisData);
		}
	}
	result.ramUsage += samples.size() * sizeof(AudioSample);
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
