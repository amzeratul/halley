#include "audio_event.h"
#include "audio_engine.h"
#include "audio_clip.h"
#include "halley/resources/resource_data.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/config_file.h"
#include "halley/text/string_converter.h"
#include "halley/core/resources/resources.h"
#include "audio_source_clip.h"
#include "audio_filter_resample.h"
#include "behaviours/audio_voice_dynamics_behaviour.h"
#include "halley/support/logger.h"

using namespace Halley;

AudioEvent::AudioEvent() = default;

AudioEvent::AudioEvent(const ConfigNode& config)
{
	if (config.hasKey("actions")) {
		for (auto& actionNode: config["actions"]) {
			const auto type = fromString<AudioEventActionType>(actionNode["type"].asString());
			if (type == AudioEventActionType::Play) {
				actions.push_back(std::make_unique<AudioEventActionPlay>(*this, actionNode));
			}
		}
	}
}

void AudioEvent::run(AudioEngine& engine, uint32_t id, const AudioPosition& position) const
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
			auto newAction = std::make_unique<AudioEventActionPlay>(*this);
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
	auto staticData = loader.getStatic();
	Deserializer s(staticData->getSpan());
	auto event = std::make_shared<AudioEvent>();
	s >> *event;
	event->loadDependencies(loader.getResources());
	return event;
}

AudioEventActionPlay::AudioEventActionPlay(AudioEvent& event)
	: event(event)
{
}

AudioEventActionPlay::AudioEventActionPlay(AudioEvent& event, const ConfigNode& node)
	: event(event)
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

	minimumSpace = node["minimumSpace"].asFloat(0.0f);
	delay = node["delay"].asFloat(0.0f);
	loop = node["loop"].asBool(false);

	if (node.hasKey("dynamics")) {
		dynamics = AudioDynamicsConfig(node["dynamics"]);
	}
}

void AudioEventActionPlay::run(AudioEngine& engine, uint32_t id, const AudioPosition& position) const
{
	if (clips.empty()) {
		return;
	}

	if (clips.size() != clipData.size()) {
		throw Exception("AudioEvent has not had its dependencies loaded correctly.", HalleyExceptions::AudioEngine);
	}

	auto& rng = engine.getRNG();
	const int clipN = rng.getInt(0, static_cast<int>(clips.size()) - 1);
	auto clip = clipData[clipN];

	if (!clip) {
		return;
	}

	const float curVolume = rng.getFloat(volume.start, volume.end);
	const float curPitch = clamp(rng.getFloat(pitch.start, pitch.end), 0.1f, 2.0f);

	constexpr int sampleRate = 48000;
	std::shared_ptr<AudioSource> source = std::make_shared<AudioSourceClip>(clip, loop, lround(delay * sampleRate));
	if (std::abs(curPitch - 1.0f) > 0.01f) {
		source = std::make_shared<AudioFilterResample>(source, int(lround(sampleRate * curPitch)), sampleRate, engine.getPool());
	}

	auto voice = std::make_unique<AudioVoice>(source, position, curVolume, engine.getGroupId(group));
	if (dynamics) {
		voice->addBehaviour(std::make_unique<AudioVoiceDynamicsBehaviour>(dynamics.value(), engine));
	}
	
	engine.addEmitter(id, std::move(voice));
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
	s << minimumSpace;
	s << loop;
	s << dynamics;
}

void AudioEventActionPlay::deserialize(Deserializer& s)
{
	s >> clips;
	s >> group;
	s >> pitch;
	s >> volume;
	s >> delay;
	s >> minimumSpace;
	s >> loop;
	s >> dynamics;
}

void AudioEventActionPlay::loadDependencies(const Resources& resources)
{
	if (clipData.size() != clips.size()) {
		clipData.clear();
		clipData.reserve(clips.size());
			
		for (auto& c: clips) {
			if (resources.exists<AudioClip>(c)) {
				clipData.push_back(resources.get<AudioClip>(c));
			} else {
				Logger::logError("AudioClip not found: \"" + c + "\", needed by \"" + event.getAssetId() + "\".");
				clipData.push_back(std::shared_ptr<AudioClip>());
			}
		}
	}
}

void AudioEvent::loadDependencies(Resources& resources) const
{
	for (auto& a: actions) {
		a->loadDependencies(resources);
	}
}
