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
	std::vector<SpatialSource> sources;
	sources.emplace_back(pos, referenceDistance, maxDistance);
	return makePositional(std::move(sources));
}

AudioPosition AudioPosition::makePositional(std::vector<SpatialSource> sources)
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
	return std::sin(std::max(0.0f, 1.0f - 0.5f * std::abs(srcPan - dstPan)) * piOverTwo);
}

void AudioPosition::setMix(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const
{
	if (isPannable) {
		Expects(nSrcChannels == 1);
		if (isUI) {
			setMixUI(dstChannels, dst, gain, listener);
		} else {
			setMixPositional(dstChannels, dst, gain, listener);
		}
	} else {
		setMixFixed(nSrcChannels, dstChannels, dst, gain, listener);
	}
}

void AudioPosition::setMixFixed(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const
{
	const size_t nDstChannels = size_t(dstChannels.size());
	Expects(nSrcChannels == 2);
	for (size_t i = 0; i < nSrcChannels; ++i) {
		float srcPan = i == 0 ? -1.0f : 1.0f;
		for (size_t j = 0; j < nDstChannels; ++j) {
			dst[i * nSrcChannels + j] = gain2DPan(srcPan, dstChannels[j].pan) * gain;
		}
	}
}

void AudioPosition::setMixUI(gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const
{
	const size_t nDstChannels = size_t(dstChannels.size());
	for (size_t i = 0; i < nDstChannels; ++i) {
		dst[i] = gain2DPan(pan, dstChannels[i].pan) * gain;
	}
}

void AudioPosition::setMixPositional(gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const
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
		auto delta = sources[0].pos - listener.position;
		resultPan = clamp(delta.x * 0.01f, -1.0f, 1.0f);
		const float len = delta.length();
		proximity = 1.0f - clamp((len - sources[0].referenceDistance) / (sources[0].maxDistance - sources[0].referenceDistance), 0.0f, 1.0f);
	} else {
		// Multiple sources, average them
		float panAccum = 0;
		float proximityAccum = 0;

		for (auto& s: sources) {
			auto delta = s.pos - listener.position;
			const float localPan = clamp(delta.x * 0.01f, -1.0f, 1.0f);
			const float len = delta.length();
			const float proximity = 1.0f - clamp((len - s.referenceDistance) / (s.maxDistance - s.referenceDistance), 0.0f, 1.0f);

			panAccum += proximity * localPan;
			proximityAccum += proximity;
		}

		if (proximityAccum > 0.01f) {
			resultPan = panAccum / proximityAccum;
			proximity = clamp(proximityAccum, 0.0f, 1.0f);
		}
	}

	for (size_t i = 0; i < nDstChannels; ++i) {
		dst[i] = gain2DPan(resultPan, dstChannels[i].pan) * gain * proximity;
	}
}
