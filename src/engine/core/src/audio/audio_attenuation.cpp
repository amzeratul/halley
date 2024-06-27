#include "halley/audio/audio_attenuation.h"
using namespace Halley;

AudioAttenuation::AudioAttenuation(const ConfigNode& node)
{
	referenceDistance = node["referenceDistance"].asFloat(100);
	maximumDistance = node["maximumDistance"].asFloat(100);
	curve = node["curve"].asEnum(AudioAttenuationCurve::Linear);
}

ConfigNode AudioAttenuation::toConfigNode() const
{
	ConfigNode::MapType result;
	result["referenceDistance"] = referenceDistance;
	result["maximumDistance"] = maximumDistance;
	result["curve"] = curve;
	return result;
}

void AudioAttenuation::serialize(Serializer& s) const
{
	s << referenceDistance;
	s << maximumDistance;
	s << curve;
}

void AudioAttenuation::deserialize(Deserializer& s)
{
	s >> referenceDistance;
	s >> maximumDistance;
	s >> curve;
}
