#pragma once

#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"

namespace Halley {
    class RenderGraphDefinition : public Resource {
    public:
    	RenderGraphDefinition();
        explicit RenderGraphDefinition(const ConfigNode& config);
    	
        static std::unique_ptr<RenderGraphDefinition> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::RenderGraphDefinition; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
    };
}
