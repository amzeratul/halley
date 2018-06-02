#include "audio_event.h"
#include "audio_engine.h"
#include "audio_clip.h"
#include "halley/resources/resource_data.h"
#include "halley/file/byte_serializer.h"
#include "halley/file_formats/config_file.h"
#include "halley/text/string_converter.h"
#include "halley/core/resources/resources.h"

using namespace Halley;

AudioEvent::AudioEvent() = default;

AudioEvent::AudioEvent(const ConfigNode& config)
{
	if (config.hasKey("actions")) {
		for (auto& actionNode: config["actions"]) {
			const auto type = fromString<AudioEventActionType>(actionNode["type"].asString());
			if (type == AudioEventActionType::Play) {
				actions.push_back(std::make_unique<AudioEventActionPlay>(actionNode));
			}
		}
	}
}

void AudioEvent::run(AudioEngine& engine, size_t id, const AudioPosition& position) const
{
	for (auto& a: actions) {
		a->run(engine, id, position);
	}
}

void AudioEvent::serialize(Serializer& s) const
{
	s << uint32_t(actions.size());
	for (auto& a: actions) {
		s << toString(a->getType());
		s << *a;
	}
}

void AudioEvent::deserialize(Deserializer& s)
{
	uint32_t size;
	s >> size;
	for (uint32_t i = 0; i < size; ++i) {
		String name;
		s >> name;
		auto type = fromString<AudioEventActionType>(name);
		if (type == AudioEventActionType::Play) {
			auto newAction = std::make_unique<AudioEventActionPlay>();
			s >> *newAction;
			actions.push_back(std::move(newAction));
		}
	}
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

AudioEventActionPlay::AudioEventActionPlay() = default;

AudioEventActionPlay::AudioEventActionPlay(const ConfigNode& node)
{
	group = node["group"].asString("");
	if (node.hasKey("clips")) {
		for (auto& clipNode: node["clips"]) {
			clips.push_back(clipNode.asString());
		}
	}
	
	if (node["pitch"].getType() == ConfigNodeType::Sequence) {
		auto seq = node["pitch"].asSequence();
		pitch = Range<float>(seq.at(0).asFloat(), seq.at(1).asFloat());
	} else {
		float val = node["pitch"].asFloat(1.0f);
		pitch = Range<float>(val, val);
	}

	if (node["volume"].getType() == ConfigNodeType::Sequence) {
		auto seq = node["volume"].asSequence();
		volume = Range<float>(seq.at(0).asFloat(), seq.at(1).asFloat());
	} else {
		float val = node["volume"].asFloat(1.0f);
		volume = Range<float>(val, val);
	}
	
	delay = node["delay"].asFloat(0.0f);
	loop = node["loop"].asBool(false);
}

void AudioEventActionPlay::run(AudioEngine& engine, size_t id, const AudioPosition& position) const
{
	if (clips.empty()) {
		return;
	}

	auto& rng = engine.getRNG();
	int clipN = rng.getInt(0, int(clips.size()) - 1);
	auto clip = engine.getResources().get<AudioClip>(clips[clipN]);

	const float curVolume = rng.getFloat(volume.s, volume.e);
	const float curPitch = rng.getFloat(pitch.s, pitch.e);

	engine.play(id, clip, position, curVolume, loop, curPitch);
}

AudioEventActionType AudioEventActionPlay::getType() const
{
	return AudioEventActionType::Play;
}

void AudioEventActionPlay::serialize(Serializer& s) const
{
	s << clips;
	s << group;
	s << pitch;
	s << volume;
	s << delay;
	s << loop;
}

void AudioEventActionPlay::deserialize(Deserializer& s)
{
	s >> clips;
	s >> group;
	s >> pitch;
	s >> volume;
	s >> delay;
	s >> loop;
}
