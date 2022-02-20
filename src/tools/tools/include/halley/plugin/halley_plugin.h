#pragma once

#include "iasset_importer.h"

namespace Halley {
    class IHalleyPlugin {
    public:
		virtual ~IHalleyPlugin() {}
		virtual bool isDebug() const = 0;
		virtual const char* getName() const = 0;
        virtual Vector<String> getSupportedPlatforms() const { return {"*"}; }
		virtual std::unique_ptr<IAssetImporter> getAssetImporter(ImportAssetType /*type*/) { return {}; }
    };
}

// Implement:
// extern "C" DLLEXPORT IHalleyPlugin* createHalleyPlugin();
// extern "C" DLLEXPORT void destroyHalleyPlugin(IHalleyPlugin* plugin);
