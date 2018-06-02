#pragma once
#include "halley/resources/resource.h"

namespace Halley
{
	class ConfigNode;
	class ConfigFile;
	class ResourceLoader;

	class AudioEvent : public Resource
	{
	public:
		AudioEvent();
		explicit AudioEvent(const ConfigNode& config);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void reload(Resource&& resource) override;
		static std::shared_ptr<AudioEvent> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioEvent; }
	};
}
