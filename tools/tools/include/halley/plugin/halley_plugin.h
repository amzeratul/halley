#pragma once

#include "iasset_importer.h"

namespace Halley {
    class IHalleyPlugin {
    public:
		virtual ~IHalleyPlugin() {}
		virtual const char* getName() const = 0;
        virtual std::vector<String> getSupportedPlatforms() const { return {"*"}; }
		virtual std::unique_ptr<IAssetImporter> getAssetImporter(AssetType type) { return {}; }
    };
}

// Implement:
// extern "C" DLLEXPORT IHalleyPlugin* createHalleyPlugin();
// extern "C" DLLEXPORT void destroyHalleyPlugin(IHalleyPlugin* plugin);
