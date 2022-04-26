#pragma once
#include "audio_properties.h"

namespace Halley {
    class GameProperties : public Resource {
    public:
        GameProperties() = default;
		GameProperties(const ConfigNode& node);

		ConfigNode toConfigNode() const;
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		constexpr static AssetType getAssetType() { return AssetType::GameProperties; }
		static std::unique_ptr<GameProperties> loadResource(ResourceLoader& loader);
		void reload(Resource&& resource) override;

		const AudioProperties& getAudioProperties() const;
		AudioProperties& getAudioProperties();

    private:
		AudioProperties audioProperties;
    };
}
