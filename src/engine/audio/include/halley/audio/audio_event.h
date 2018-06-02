#pragma once
#include "halley/resources/resource.h"

namespace Halley
{
	class AudioPosition;
	class AudioEngine;
	class ConfigNode;
	class ConfigFile;
	class ResourceLoader;

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
	};
}
