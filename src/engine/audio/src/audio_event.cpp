#include "audio_event.h"
#include "halley/resources/resource_data.h"
#include "halley/file/byte_serializer.h"
#include "halley/file_formats/config_file.h"

using namespace Halley;

AudioEvent::AudioEvent()
{
}

AudioEvent::AudioEvent(const ConfigNode& config)
{
	
}

void AudioEvent::serialize(Serializer& s) const
{
}

void AudioEvent::deserialize(Deserializer& s)
{
}

void AudioEvent::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<AudioEvent&>(resource));
}

std::shared_ptr<AudioEvent> AudioEvent::loadResource(ResourceLoader& loader)
{
	Deserializer s(loader.getStatic()->getSpan());
	auto event = std::make_shared<AudioEvent>();
	s >> *event;
	return event;
}
