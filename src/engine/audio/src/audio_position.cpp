#include "audio_position.h"
#include "halley/core/api/audio_api.h"

using namespace Halley;


AudioPosition::SpatialSource::SpatialSource()
	: referenceDistance(200)
	, maxDistance(400)
{
}

AudioPosition::SpatialSource::SpatialSource(Vector2f pos, float referenceDistance, float maxDistance)
	: pos(pos)
	, referenceDistance(referenceDistance)
	, maxDistance(maxDistance)
{
}

AudioPosition::SpatialSource::SpatialSource(Vector3f pos, float referenceDistance, float maxDistance)
	: pos(pos)
	, referenceDistance(referenceDistance)
	, maxDistance(maxDistance)
{
}

AudioPosition::AudioPosition()
	: isUI(true)
	, isPannable(false)
{
}

AudioPosition AudioPosition::makeUI(float pan)
{
	auto result = AudioPosition();
	result.pan = pan;
	result.isUI = true;
	result.isPannable = true;
	return result;
}

AudioPosition AudioPosition::makePositional(Vector2f pos, float referenceDistance, float maxDistance)
{
	return makePositional(Vector3f(pos), referenceDistance, maxDistance);
}

AudioPosition AudioPosition::makePositional(Vector3f pos, float referenceDistance, float maxDistance)
{
	Vector<SpatialSource> sources;
	sources.emplace_back(pos, referenceDistance, maxDistance);
	return makePositional(std::move(sources));
}

AudioPosition AudioPosition::makePositional(Vector<SpatialSource> sources)
{
	auto result = AudioPosition();

	result.isUI = false;
	result.isPannable = true;
	result.sources = std::move(sources);

	for (auto& s: result.sources) {
		s.referenceDistance = std::max(0.1f, s.referenceDistance);
		s.maxDistance = std::max(s.referenceDistance + 0.1f, s.maxDistance);
	}

	return result;
}

AudioPosition AudioPosition::makeFixed()
{
	auto result = AudioPosition();
	result.isUI = true;
	result.isPannable = false;
	return result;
}

void AudioPosition::setPosition(Vector3f position)
{
	if (!sources.empty()) {
		sources[0].pos = position;
	}
}

static float gain2DPan(float srcPan, float dstPan)
{
	constexpr float piOverTwo = 3.1415926535897932384626433832795f / 2.0f;
	const float panDistance = std::abs(srcPan - dstPan);
	return std::sin(std::max(0.0f, 1.0f - 0.5f * panDistance) * piOverTwo);
}

static void getPanAndDistance(Vector3f pos, const AudioListenerData& listener, float& pan, float& distance)
{
	auto delta = pos - listener.position;
	pan = clamp(delta.x / listener.referenceDistance, -1.0f, 1.0f);
	distance = delta.length();
}

void AudioPosition::setMix(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const
{
	if (isPannable) {
		if (isUI) {
			setMixUI(dstChannels, dst, gain, listener);
		} else {
			setMixPositional(nSrcChannels, dstChannels, dst, gain, listener);
		}
	} else {
		setMixFixed(nSrcChannels, dstChannels, dst, gain, listener);
	}
}

void AudioPosition::setMixFixed(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const
{
	const size_t nDstChannels = size_t(dstChannels.size());
	Expects(nSrcChannels == 1 || nSrcChannels == 2);
	for (size_t srcChannel = 0; srcChannel < nSrcChannels; ++srcChannel) {
		const float srcPan = nSrcChannels == 1 ? 0.0f : (srcChannel == 0 ? -1.0f : 1.0f);
		for (size_t dstChannel = 0; dstChannel < nDstChannels; ++dstChannel) {
			dst[srcChannel * nSrcChannels + dstChannel] = gain2DPan(srcPan, dstChannels[dstChannel].pan) * gain * dstChannels[srcChannel].gain;
		}
	}
}

void AudioPosition::setMixUI(gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const
{
	const size_t nDstChannels = size_t(dstChannels.size());
	for (size_t i = 0; i < nDstChannels; ++i) {
		dst[i] = gain2DPan(pan, dstChannels[i].pan) * gain * dstChannels[i].gain;
	}
}

void AudioPosition::setMixPositional(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const
{
	const size_t nDstChannels = size_t(dstChannels.size());
	if (sources.empty()) {
		// No sources, don't emit anything
		for (size_t i = 0; i < nDstChannels; ++i) {
			dst[i] = 0;
		}
		return;
	}

	// Proximity means 1 within the reference distance, 0 outside the maximum distance, and between 0 and 1 between them
	float proximity = 0;
	float resultPan = 0;

	if (sources.size() == 1) {
		// One source, do the simple algorithm
		float len;
		getPanAndDistance(sources[0].pos, listener, resultPan, len);
		proximity = 1.0f - clamp((len - sources[0].referenceDistance) / (sources[0].maxDistance - sources[0].referenceDistance), 0.0f, 1.0f);
	} else {
		// Multiple sources, average them
		float panAccum = 0;
		float proximityAccum = 0;

		for (auto& s: sources) {
			float localPan;
			float len;
			getPanAndDistance(s.pos, listener, localPan, len);
			const float localProximity = 1.0f - clamp((len - s.referenceDistance) / (s.maxDistance - s.referenceDistance), 0.0f, 1.0f);

			panAccum += localProximity * localPan;
			proximityAccum += localProximity;
		}

		if (proximityAccum > 0.01f) {
			resultPan = panAccum / proximityAccum;
			proximity = clamp(proximityAccum, 0.0f, 1.0f);
		}
	}

	for (size_t srcChannel = 0; srcChannel < nSrcChannels; ++srcChannel) {
		// Read to buffer
		for (size_t dstChannel = 0; dstChannel < nDstChannels; ++dstChannel) {
			// Compute mix
			const size_t mixIndex = (srcChannel * nSrcChannels) + dstChannel;
			dst[mixIndex] = gain2DPan(resultPan, dstChannels[dstChannel].pan) * gain * proximity * dstChannels[dstChannel].gain;
		}
	}
}
