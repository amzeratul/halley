#include "audio_sub_object.h"

#include "audio_engine.h"
#include "halley/bytes/byte_serializer.h"
#include "sub_objects/audio_sub_object_clips.h"
#include "sub_objects/audio_sub_object_layers.h"
#include "sub_objects/audio_sub_object_sequence.h"
#include "sub_objects/audio_sub_object_switch.h"

using namespace Halley;

std::unique_ptr<IAudioSubObject> IAudioSubObject::makeSubObject(AudioSubObjectType type)
{
	switch (type) {
	case AudioSubObjectType::Clips:
		return std::make_unique<AudioSubObjectClips>();
	case AudioSubObjectType::Layers:
		return std::make_unique<AudioSubObjectLayers>();
	case AudioSubObjectType::Switch:
		return std::make_unique<AudioSubObjectSwitch>();
	case AudioSubObjectType::Sequence:
		return std::make_unique<AudioSubObjectSequence>();
	}
	return {};
}

std::unique_ptr<IAudioSubObject> IAudioSubObject::makeSubObject(const ConfigNode& node)
{
	const auto type = fromString<AudioSubObjectType>(node["type"].asString());
	auto obj = makeSubObject(type);
	if (obj) {
		obj->load(node);
	}
	return obj;
}

size_t IAudioSubObject::getNumSubObjects() const
{
	return 0;
}

const AudioSubObjectHandle& IAudioSubObject::getSubObject(size_t n) const
{
	throw Exception("Sub-object index out of range", HalleyExceptions::AudioEngine);
}

Vector<String> IAudioSubObject::getSubCategories() const
{
	return {};
}

String IAudioSubObject::getSubObjectCategory(size_t n) const
{
	return "";
}

gsl::span<const String> IAudioSubObject::getClips() const
{
	return {};
}

bool IAudioSubObject::canCollapseToClip() const
{
	return false;
}

AudioSubObjectHandle::AudioSubObjectHandle(std::unique_ptr<IAudioSubObject> obj)
	: obj(std::move(obj))
{
}

AudioSubObjectHandle::AudioSubObjectHandle(const ConfigNode& node)
{
	obj = IAudioSubObject::makeSubObject(node);
}

AudioSubObjectHandle::AudioSubObjectHandle(const AudioSubObjectHandle& other)
{
	*this = other;
}

AudioSubObjectHandle& AudioSubObjectHandle::operator=(const AudioSubObjectHandle& other)
{
	// HACK
	auto s = Deserializer(Serializer::toBytes(other));
	deserialize(s);
	return *this;
}

ConfigNode AudioSubObjectHandle::toConfigNode() const
{
	return obj->toConfigNode();
}

IAudioSubObject& AudioSubObjectHandle::getObject()
{
	return *obj;
}

const IAudioSubObject& AudioSubObjectHandle::getObject() const
{
	return *obj;
}

IAudioSubObject& AudioSubObjectHandle::operator*()
{
	return *obj;
}

IAudioSubObject& AudioSubObjectHandle::operator*() const
{
	return *obj;
}

IAudioSubObject* AudioSubObjectHandle::operator->()
{
	return obj.get();
}

IAudioSubObject* AudioSubObjectHandle::operator->() const
{
	return obj.get();
}

bool AudioSubObjectHandle::hasValue() const
{
	return !!obj;
}

void AudioSubObjectHandle::serialize(Serializer& s) const
{
	if (obj) {
		s << static_cast<int>(obj->getType());
		obj->serialize(s);
	} else {
		s << static_cast<int>(AudioSubObjectType::None);
	}
}

void AudioSubObjectHandle::deserialize(Deserializer& s)
{
	int type;
	s >> type;
	obj = IAudioSubObject::makeSubObject(static_cast<AudioSubObjectType>(type));
	if (obj) {
		obj->deserialize(s);
	}
}

