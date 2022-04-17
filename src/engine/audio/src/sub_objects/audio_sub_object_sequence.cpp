#include "audio_sub_object_sequence.h"
using namespace Halley;

void AudioSubObjectSequence::load(const ConfigNode& node)
{
	// TODO
}

ConfigNode AudioSubObjectSequence::toConfigNode() const
{
	ConfigNode::MapType result;
	// TODO
	return result;
}

std::unique_ptr<AudioSource> AudioSubObjectSequence::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	// TODO
	return {};
}

void AudioSubObjectSequence::loadDependencies(Resources& resources)
{
	// TODO
}

void AudioSubObjectSequence::serialize(Serializer& s) const
{
	// TODO
}

void AudioSubObjectSequence::deserialize(Deserializer& s)
{
	// TODO
}
