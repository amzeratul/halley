#include "audio_object.h"

#include "halley/bytes/byte_serializer.h"

using namespace Halley;

AudioObject::AudioObject()
{}

AudioObject::AudioObject(const ConfigNode& config)
{
	// TODO
}

void AudioObject::serialize(Serializer& s) const
{
	// TODO
}

void AudioObject::deserialize(Deserializer& s)
{
	// TODO
}

void AudioObject::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<AudioObject&>(resource));
}

std::shared_ptr<AudioObject> AudioObject::loadResource(ResourceLoader& loader)
{
	auto staticData = loader.getStatic(false);
	if (!staticData) {
		return {};
	}
	
	Deserializer s(staticData->getSpan());
	auto object = std::make_shared<AudioObject>();
	s >> *object;
	object->loadDependencies(loader.getResources());
	return object;
}

void AudioObject::loadDependencies(Resources& resources) const
{
	
}
