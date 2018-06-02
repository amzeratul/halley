#pragma once
#include "halley/resources/resource.h"
#include "halley/maths/range.h"
#include "halley/data_structures/maybe.h"

namespace Halley
{
	class AudioPosition;
	class AudioEngine;
	class ConfigNode;
	class ConfigFile;
	class ResourceLoader;
	class IAudioEventAction;

	class AudioEvent : public Resource
	{
	public:
		AudioEvent();
		explicit AudioEvent(const ConfigNode& config);

		void run(AudioEngine& engine, size_t id, const AudioPosition& position) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void reload(Resource&& resource) override;
		static std::shared_ptr<AudioEvent> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioEvent; }

	private:
		std::vector<std::unique_ptr<const IAudioEventAction>> actions;
	};

	enum class AudioEventActionType
	{
		Play
	};

	template <>
	struct EnumNames<AudioEventActionType> {
		constexpr std::array<const char*, 1> operator()() const {
			return{{
				"play"
			}};
		}
	};

	class IAudioEventAction
	{
	public:
		virtual ~IAudioEventAction() {}
		virtual void run(AudioEngine& engine, size_t id, const AudioPosition& position) const = 0;
		virtual AudioEventActionType getType() const = 0;

		virtual void serialize(Serializer& s) const = 0;
		virtual void deserialize(Deserializer& s) = 0;
	};

	class AudioEventActionPlay : public IAudioEventAction
	{
	public:
		AudioEventActionPlay();
		explicit AudioEventActionPlay(const ConfigNode& config);

		void run(AudioEngine& engine, size_t id, const AudioPosition& position) const override;
		AudioEventActionType getType() const override;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

	private:
		std::vector<String> clips;
		String group;
		Range<float> pitch;
		Range<float> volume;
		float delay = 0.0f;
		bool loop = false;
	};
}
