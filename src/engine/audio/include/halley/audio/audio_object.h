#pragma once

#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"
#include "halley/core/api/audio_api.h"

namespace Halley {
    class AudioObject final : public Resource {
    public:
    	AudioObject();
		explicit AudioObject(const ConfigNode& config);

    	void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void reload(Resource&& resource) override;
		static std::shared_ptr<AudioObject> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioObject; }

    private:
    	void loadDependencies(Resources& resources) const;
    };
}
