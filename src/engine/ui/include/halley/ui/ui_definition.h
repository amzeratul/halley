#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/file_formats/config_file.h"

namespace Halley {
    class UIDefinition : public Resource {
    public:
        UIDefinition();
        UIDefinition(ConfigNode node);
        UIDefinition(ConfigFile file);

    	static std::unique_ptr<UIDefinition> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::UIDefinition; }

		void reload(Resource&& resource) override;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

    	const ConfigNode& getRoot() const;
        ConfigNode& getRoot();

    private:
        ConfigFile data;
    };
}
