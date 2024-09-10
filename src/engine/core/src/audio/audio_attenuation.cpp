#include "halley/audio/audio_attenuation.h"
using namespace Halley;

AudioAttenuation::AudioAttenuation(const ConfigNode& node)
{
	referenceDistance = node["referenceDistance"].asFloat(100);
	maximumDistance = node["maximumDistance"].asFloat(100);
	rollOffFactor = node["rollOffFactor"].asFloat(1);
	curve = node["curve"].asEnum(AudioAttenuationCurve::Linear);
}

AudioAttenuation::AudioAttenuation(float refDistance, float maxDistance, float rollOffFactor, AudioAttenuationCurve curve)
	: referenceDistance(refDistance)
	, maximumDistance(maxDistance)
	, rollOffFactor(rollOffFactor)
	, curve(curve)
{
}

ConfigNode AudioAttenuation::toConfigNode() const
{
	ConfigNode::MapType result;
	result["referenceDistance"] = referenceDistance;
	result["maximumDistance"] = maximumDistance;
	result["rollOffFactor"] = rollOffFactor;
	result["curve"] = curve;
	return result;
}

void AudioAttenuation::serialize(Serializer& s) const
{
	s << referenceDistance;
	s << maximumDistance;
	s << rollOffFactor;
	s << curve;
}

void AudioAttenuation::deserialize(Deserializer& s)
{
	s >> referenceDistance;
	s >> maximumDistance;
	s >> rollOffFactor;
	s >> curve;
}

float AudioAttenuation::getProximity(float distance) const
{
	assert(rollOffFactor > 0.000001f);

	distance = clamp(distance, referenceDistance, maximumDistance);
	const float linearBit = clamp((1 - (distance / maximumDistance)) * 10, 0.0f, 1.0f);

	switch (curve) {
	case AudioAttenuationCurve::None:
		return 1.0f;

	case AudioAttenuationCurve::Linear:
		return 1.0f - clamp(rollOffFactor * (distance - referenceDistance) / (maximumDistance - referenceDistance), 0.0f, 1.0f);

	case AudioAttenuationCurve::InvDistance:
		return referenceDistance / (referenceDistance + rollOffFactor * (distance - referenceDistance)) * linearBit;

	case AudioAttenuationCurve::Exponential:
		return std::pow(distance / referenceDistance, -rollOffFactor) * linearBit;
	}

	return 1.0f;
}
