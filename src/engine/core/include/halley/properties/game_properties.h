#pragma once
#include "audio_properties.h"

namespace Halley {
    class GameProperties : public Resource {
    public:
        GameProperties() = default;
		GameProperties(const ConfigNode& node);
        explicit GameProperties(Path path);

		ConfigNode toConfigNode() const;
		void save() const;
		void load();
		void load(const ConfigNode& node);
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		constexpr static AssetType getAssetType() { return AssetType::GameProperties; }
		static std::unique_ptr<GameProperties> loadResource(ResourceLoader& loader);
		void reload(Resource&& resource) override;

		const AudioProperties& getAudioProperties() const;
		AudioProperties& getAudioProperties();

    private:
		AudioProperties audioProperties;
		Path path;
    };
}
