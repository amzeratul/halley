#include "halley/audio/audio_position.h"
#include "halley/api/audio_api.h"
#include "halley/audio/audio_object.h"

using namespace Halley;


AudioPosition::SpatialSource::SpatialSource(Vector2f pos, Vector2f vel, float referenceDistance, float maxDistance)
	: pos(pos)
	, velocity(vel)
	, attenuation(AudioAttenuation(referenceDistance, maxDistance))
	, polygon(std::nullopt)
{
}

AudioPosition::SpatialSource::SpatialSource(Vector3f pos, Vector3f vel, float referenceDistance, float maxDistance)
	: pos(pos)
	, velocity(vel)
	, attenuation(AudioAttenuation(referenceDistance, maxDistance))
	, polygon(std::nullopt)
{
}

AudioPosition::SpatialSource::SpatialSource(Vector2f pos, Vector2f vel, AudioAttenuation attenuation)
	: pos(pos)
	, velocity(vel)
	, attenuation(attenuation)
	, polygon(std::nullopt)
{
}

AudioPosition::SpatialSource::SpatialSource(Vector2f pos, Polygon polygon, Vector2f vel, AudioAttenuation attenuation)
	: pos(pos)
	, velocity(vel)
	, attenuation(attenuation)
	, polygon(std::move(polygon))
{
}

AudioPosition::SpatialSource::SpatialSource(Vector3f pos, Vector3f vel, AudioAttenuation attenuation)
	: pos(pos)
	, velocity(vel)
	, attenuation(attenuation)
	, polygon(std::nullopt)
{
}

AudioPosition::AudioPosition()
	: isUI(true)
	, isPannable(false)
{
	static_assert(sources.sbo_max_objects() == 1);
}

AudioPosition AudioPosition::makeUI(float pan)
{
	auto result = AudioPosition();
	result.uiPan = pan;
	result.isUI = true;
	result.isPannable = true;
	return result;
}

AudioPosition AudioPosition::makePositional(Vector2f pos, float referenceDistance, float maxDistance, Vector2f velocity)
{
	return makePositional(Vector3f(pos), AudioAttenuation(referenceDistance, maxDistance), Vector3f(velocity));
}

AudioPosition AudioPosition::makePositional(Vector3f pos, float referenceDistance, float maxDistance, Vector3f velocity)
{
	Vector<SpatialSource> sources;
	sources.emplace_back(pos, velocity, AudioAttenuation(referenceDistance, maxDistance));
	return makePositional(std::move(sources));
}

AudioPosition AudioPosition::makePositional(Vector2f pos, AudioAttenuation attenuation, Vector2f velocity)
{
	Vector<SpatialSource> sources;
	sources.emplace_back(pos, velocity, attenuation);
	return makePositional(std::move(sources));
}

AudioPosition AudioPosition::makePositional(Vector2f pos, Polygon polygon, AudioAttenuation attenuation, Vector2f velocity)
{
	Vector<SpatialSource> sources;
	sources.emplace_back(pos, std::move(polygon), velocity, attenuation);
	return makePositional(std::move(sources));
}

AudioPosition AudioPosition::makePositional(Vector3f pos, AudioAttenuation attenuation, Vector3f velocity)
{
	Vector<SpatialSource> sources;
	sources.emplace_back(pos, velocity, attenuation);
	return makePositional(std::move(sources));
}

AudioPosition AudioPosition::makePositional(Vector<SpatialSource> sources)
{
	auto result = AudioPosition();

	result.isUI = false;
	result.isPannable = true;
	result.sources = std::move(sources);

	for (auto& s: result.sources) {
		s.attenuation.referenceDistance = std::max(0.1f, s.attenuation.referenceDistance);
		s.attenuation.maximumDistance = std::max(s.attenuation.referenceDistance + 0.1f, s.attenuation.maximumDistance);
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

float AudioPosition::getDopplerShift(const AudioListenerData& listener) const
{
	if (sources.empty()) {
		return 0.0f;
	}

	// TODO: use closest source?
	const auto srcVel = sources[0].velocity;
	const auto listenerVel = listener.velocity;
	const auto dir = (sources[0].pos - listener.position).normalized(); // Towards source
	const auto c = listener.speedOfSound;

	const auto srcSpeed = clamp(srcVel.dot(dir), -0.8f * c, 0.8f * c);
	const auto listenerSpeed = clamp(listenerVel.dot(dir), -0.8f * c, 0.8f * c);

	return (c + listenerSpeed) / (c + srcSpeed) - 1.0f;
}

namespace {
	float gain2DPan(float srcPan, float dstPan)
	{
		constexpr float piOverTwo = 3.1415926535897932384626433832795f / 2.0f;
		const float panDistance = std::abs(srcPan - dstPan);
		return std::sin(std::max(0.0f, 1.0f - 0.5f * panDistance) * piOverTwo);
	}

	void getPanAndDistance(Vector3f sourcePos, const AudioListenerData& listener, float& pan, float& distance)
	{
		auto delta = sourcePos - listener.position;
		pan = clamp(delta.x / listener.referenceDistance, -1.0f, 1.0f);
		distance = delta.length();
	}

	Vector3f getSourcePosition(Vector3f sourcePos, const Polygon& polygon, const AudioListenerData& listener, float maxDist)
	{
		if (polygon.isValid()) {
			const auto relPos = listener.position.xy() - sourcePos.xy();

			// Avoid the expensive polygon checks if too far away
			const auto circle = polygon.getBoundingCircle();
			const float dist2 = (relPos - circle.getCentre()).squaredLength();
			const float maxRange = circle.getRadius() + maxDist;
			if (dist2 > maxRange * maxRange) {
				return sourcePos + Vector3f(circle.getCentre(), 0);
			}

			if (polygon.isPointInside(relPos)) {
				return listener.position;
			} else {
				return Vector3f(polygon.getClosestPoint(relPos) + sourcePos.xy(), 0);
			}
		} else {
			return sourcePos;
		}
	}

	void getPanAndDistance(Vector3f pos, const Polygon* polygon, const AudioListenerData& listener, float& pan, float& distance, float maxDist)
	{
		const auto effectivePos = polygon ? getSourcePosition(pos, *polygon, listener, maxDist) : pos;
		return getPanAndDistance(effectivePos, listener, pan, distance);
	}
}

float AudioPosition::setMix(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener, const AudioMixingProperties& mixingProperties) const
{
	if (isPannable) {
		if (isUI) {
			setMixUI(dstChannels, dst, gain, listener);
			return 0;
		} else {
			return setMixPositional(nSrcChannels, dstChannels, dst, gain, listener, mixingProperties);
		}
	} else {
		setMixFixed(nSrcChannels, dstChannels, dst, gain, listener);
		return 0;
	}
}

float AudioPosition::getDistance(const AudioListenerData& listener) const
{
	if (sources.empty()) {
		return std::numeric_limits<float>::max();
	}

	return getAttenuationAndPanPositional(listener, {}).distance;
}

void AudioPosition::setMixFixed(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener) const
{
	const size_t nDstChannels = size_t(dstChannels.size());

	if (nSrcChannels < 1 || nSrcChannels > 2) {
		Logger::logError("Unexpected number of srcChannels: " + toString(nSrcChannels));
		for (auto& d: dst) {
			d = 0;
		}
		return;
	}

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
		dst[i] = gain2DPan(uiPan, dstChannels[i].pan) * gain * dstChannels[i].gain;
	}
}

float AudioPosition::getAttenuation(const AudioListenerData& listener, const std::optional<AudioAttenuation>& attenuationOverride) const
{
	if (isPannable && !isUI) {
		return getAttenuationAndPanPositional(listener, attenuationOverride).attenuation;
	} else {
		return 1.0f;
	}
}

AudioPosition::AttenuationResult AudioPosition::getAttenuationAndPanPositional(const AudioListenerData& listener, const std::optional<AudioAttenuation>& attenuationOverride) const
{
	float attenuation = 0;
	float resultPan = 0;
	float distance = 0;

	if (sources.size() == 1) {
		// One source, do the simple algorithm
		getPanAndDistance(sources[0].pos, sources[0].polygon.get(), listener, resultPan, distance, sources[0].attenuation.maximumDistance);
		attenuation = attenuationOverride.value_or(sources[0].attenuation).getProximity(distance);
	} else {
		// Multiple sources, average them
		float panAccum = 0;
		float proximityAccum = 0;

		for (auto& s: sources) {
			float localPan;
			float len;
			getPanAndDistance(s.pos, s.polygon.get(), listener, localPan, len, s.attenuation.maximumDistance);
			const float localProximity = attenuationOverride.value_or(s.attenuation).getProximity(len);

			panAccum += localProximity * localPan;
			proximityAccum += localProximity;
			distance += len;
		}

		distance /= static_cast<float>(sources.size());

		if (proximityAccum > 0.01f) {
			resultPan = panAccum / proximityAccum;
			attenuation = clamp(proximityAccum, 0.0f, 1.0f);
		}
	}

	return { attenuation, resultPan, distance };
}

float AudioPosition::setMixPositional(size_t nSrcChannels, gsl::span<const AudioChannelData> dstChannels, gsl::span<float, 16> dst, float gain, const AudioListenerData& listener, const AudioMixingProperties& mixingProperties) const
{
	const size_t nDstChannels = size_t(dstChannels.size());
	if (sources.empty()) {
		// No sources, don't emit anything
		for (size_t i = 0; i < nDstChannels; ++i) {
			dst[i] = 0;
		}
		return std::numeric_limits<float>::max();
	}

	const auto [attenuation, centrePan, distance] = getAttenuationAndPanPositional(listener, mixingProperties.attenuationOverride);

	float halfWidth = 0;
	if (nSrcChannels == 2) {
		// Stereo width
		const float minWidth = mixingProperties.minStereoWidth.value_or(0);
		const float maxWidth = mixingProperties.maxStereoWidth.value_or(180);
		const float widthDegrees = clamp(lerp(minWidth, maxWidth, attenuation), 0.0f, 180.0f);
		halfWidth = Angle1f::fromDegrees(widthDegrees / 2).sin();
	}
	const float limitedCentrePan = clamp(centrePan, -1.0f + halfWidth, 1.0f - halfWidth); // Clamp it closer to centre if needed due to width

	for (size_t srcChannel = 0; srcChannel < nSrcChannels; ++srcChannel) {
		const float srcPan = nSrcChannels == 1 ? 0.0f : (srcChannel == 0 ? -1.0f : 1.0f);
		const float srcChannelPan = limitedCentrePan + srcPan * halfWidth;

		for (size_t dstChannel = 0; dstChannel < nDstChannels; ++dstChannel) {
			const float panGain = gain2DPan(srcChannelPan, dstChannels[dstChannel].pan);
			const size_t mixIndex = (srcChannel * nSrcChannels) + dstChannel;
			dst[mixIndex] = panGain * gain * attenuation * dstChannels[dstChannel].gain;
		}
	}

	return distance;
}
