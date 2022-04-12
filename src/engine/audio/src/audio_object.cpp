#include "audio_object.h"
#include "audio_clip.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/core/resources/resources.h"
#include "halley/data_structures/config_node.h"
#include "halley/maths/random.h"
#include "halley/support/logger.h"

using namespace Halley;

AudioObject::AudioObject()
{
	generateId();
}

AudioObject::AudioObject(const ConfigNode& config)
{
	generateId();
	// TODO
}

void AudioObject::loadLegacyEvent(const ConfigNode& node)
{
	group = node["group"].asString("");
	clips = node["clips"].asVector<String>({});
	pitch = node["pitch"].asFloatRange(Range<float>(1, 1));
	volume = node["volume"].asFloatRange(Range<float>(1, 1));
	delay = node["delay"].asFloat(0.0f);
	loop = node["loop"].asBool(false);
}

uint32_t AudioObject::getAudioObjectId() const
{
	return audioObjectId;
}

const String& AudioObject::getGroup() const
{
	return group;
}

std::shared_ptr<const AudioClip> AudioObject::getRandomClip(Random& rng) const
{
	if (clipData.empty()) {
		return {};
	}

	return rng.getRandomElement(clipData);
}

Range<float> AudioObject::getPitch() const
{
	return pitch;
}

Range<float> AudioObject::getVolume() const
{
	return volume;
}

float AudioObject::getDelay() const
{
	return delay;
}

bool AudioObject::getLoop() const
{
	return loop;
}

void AudioObject::serialize(Serializer& s) const
{
	s << clips;
	s << group;
	s << pitch;
	s << volume;
	s << delay;
	s << loop;
}

void AudioObject::deserialize(Deserializer& s)
{
	s >> clips;
	s >> group;
	s >> pitch;
	s >> volume;
	s >> delay;
	s >> loop;
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

void AudioObject::loadDependencies(Resources& resources)
{
	if (clipData.size() != clips.size()) {
		clipData.clear();
		clipData.reserve(clips.size());
			
		for (auto& c: clips) {
			if (resources.exists<AudioClip>(c)) {
				clipData.push_back(resources.get<AudioClip>(c));
			} else {
				Logger::logError("AudioClip not found: \"" + c + "\".");
				clipData.push_back(std::shared_ptr<AudioClip>());
			}
		}
	}
}

void AudioObject::generateId()
{
	static std::atomic<uint32_t> id = 1;
	
	audioObjectId = id++;
}
