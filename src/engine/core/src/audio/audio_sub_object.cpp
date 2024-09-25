#include "halley/audio/audio_sub_object.h"

#include "audio_engine.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/maths/uuid.h"
#include "halley/audio/sub_objects/audio_sub_object_clips.h"
#include "halley/audio/sub_objects/audio_sub_object_layers.h"
#include "halley/audio/sub_objects/audio_sub_object_sequence.h"
#include "halley/audio/sub_objects/audio_sub_object_switch.h"

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

template <typename T>
static void castSubObjectCopy(IAudioSubObject& dst, const IAudioSubObject& src)
{
	dynamic_cast<T&>(dst) = dynamic_cast<const T&>(src);
}

void IAudioSubObject::copySubObject(IAudioSubObject& dst, const IAudioSubObject& src)
{
	if (dst.getType() == src.getType()) {
		switch (dst.getType()) {
		case AudioSubObjectType::Clips:
			castSubObjectCopy<AudioSubObjectClips>(dst, src);
			break;
		case AudioSubObjectType::Layers:
			castSubObjectCopy<AudioSubObjectLayers>(dst, src);
			break;
		case AudioSubObjectType::Switch:
			castSubObjectCopy<AudioSubObjectSwitch>(dst, src);
			break;
		case AudioSubObjectType::Sequence:
			castSubObjectCopy<AudioSubObjectSequence>(dst, src);
			break;
		}
	} else {
		throw Exception("Cannot copy heterogeneous audio objects.", HalleyExceptions::AudioEngine);
	}
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

size_t IAudioObject::getNumSubObjects() const
{
	return 0;
}

AudioSubObjectHandle& IAudioObject::getSubObject(size_t n)
{
	throw Exception("Sub-object index out of range", HalleyExceptions::AudioEngine);
}

Vector<String> IAudioObject::getSubCategories(const AudioProperties& audioProperties) const
{
	return {};
}

String IAudioObject::getSubObjectCategory(size_t n) const
{
	return "";
}

gsl::span<const String> IAudioObject::getClips() const
{
	return {};
}

bool IAudioObject::canCollapseToClip() const
{
	return false;
}

bool IAudioObject::canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const
{
	return false;
}

void IAudioObject::addObject(AudioSubObjectHandle handle, const std::optional<String>& caseName, size_t idx)
{
}

AudioSubObjectHandle IAudioObject::removeObject(const IAudioObject* object)
{
	return AudioSubObjectHandle();
}

void IAudioObject::addClip(std::shared_ptr<const AudioClip> clip, const std::optional<String>& caseName, size_t idx)
{
	addObject(AudioSubObjectHandle(std::make_unique<AudioSubObjectClips>(std::move(clip))), caseName, idx);
}

void IAudioObject::removeClip(const String& clipId)
{
}

void IAudioObject::swapClips(size_t idxA, size_t idxB)
{
}

void IAudioObject::propagateObjectName(const String& name)
{
	setObjectName(name);
	const auto n = getNumSubObjects();
	for (size_t i = 0; i < n; ++i) {
		getSubObject(i)->propagateObjectName(name);
	}
}

AudioSubObjectHandle::AudioSubObjectHandle()
	: id(UUID::generate().toString())
{
}

AudioSubObjectHandle::AudioSubObjectHandle(std::unique_ptr<IAudioSubObject> obj)
	: id(UUID::generate().toString())
	, obj(std::move(obj))
{
}

AudioSubObjectHandle::AudioSubObjectHandle(const ConfigNode& node)
	: id(UUID::generate().toString())
	, obj(IAudioSubObject::makeSubObject(node))
{
}

AudioSubObjectHandle::AudioSubObjectHandle(const AudioSubObjectHandle& other)
{
	*this = other;
}

AudioSubObjectHandle& AudioSubObjectHandle::operator=(const AudioSubObjectHandle& other)
{
	id = other.id;
	if (hasValue() && other.hasValue() && obj->getType() == other->getType()) {
		IAudioSubObject::copySubObject(*obj, other.getObject());
	} else {
		// HACK
		auto bytes = Serializer::toBytes(other);
		auto s = Deserializer(bytes);
		deserialize(s);
	}
	return *this;
}

ConfigNode AudioSubObjectHandle::toConfigNode() const
{
	return obj->toConfigNode();
}

const String& AudioSubObjectHandle::getId() const
{
	return id;
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

