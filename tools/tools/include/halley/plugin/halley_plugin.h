#pragma once

#include "iasset_importer.h"

namespace Halley {
    class IHalleyPlugin {
    public:
		virtual ~IHalleyPlugin() {}
        virtual std::vector<String> getSupportedPlatforms() const { return {"*"}; }
		virtual std::unique_ptr<IAssetImporter> getAssetImporter(AssetType type) { return {}; }
    };
}

// Implement:
// extern "C" IHalleyPlugin* createHalleyPlugin();
// extern "C" void destroyHalleyPlugin(IHalleyPlugin* plugin);
