#include "audio_dynamics_config.h"

#include "halley/file_formats/config_file.h"
#include "halley/bytes/byte_serializer.h"

using namespace Halley;

AudioDynamicsConfig::AudioDynamicsConfig()
{
}

AudioDynamicsConfig::AudioDynamicsConfig(const ConfigNode& node)
{
	if (node.hasKey("volume")) {
		for (const auto& n: node["volume"]) {
			volume.emplace_back(n);
		}
	}
}

void AudioDynamicsConfig::serialize(Serializer& s) const
{
	s << volume;
}

void AudioDynamicsConfig::deserialize(Deserializer& s)
{
	s >> volume;
}

const std::vector<AudioDynamicsConfig::Variable>& AudioDynamicsConfig::getVolume() const
{
	return volume;
}

AudioDynamicsConfig::Variable::Variable()
{
}

AudioDynamicsConfig::Variable::Variable(const ConfigNode& node)
{
	name = node["name"].asString();
}

void AudioDynamicsConfig::Variable::serialize(Serializer& s) const
{
	s << name;
}

void AudioDynamicsConfig::Variable::deserialize(Deserializer& s)
{
	s >> name;
}

float AudioDynamicsConfig::Variable::getValue(float variable) const
{
	return variable;
}
