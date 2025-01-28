#pragma once

#include "iasset_importer.h"
#include "halley_editor_plugin.h"

namespace Halley {
    class IHalleyPlugin {
    public:
		virtual ~IHalleyPlugin() {}

		virtual bool isDebug() const = 0;
		virtual const char* getName() const = 0;
        virtual Vector<String> getSupportedPlatforms() const { return {"*"}; }

		virtual Vector<std::unique_ptr<IAssetImporter>> makeAssetImporters(ImportAssetType type) { return {}; }
		virtual std::unique_ptr<IHalleyEditorPlugin> makeHalleyEditorPlugin() { return {}; }
    };
}

// Implement:
// extern "C" DLLEXPORT IHalleyPlugin* createHalleyPlugin();
// extern "C" DLLEXPORT void destroyHalleyPlugin(IHalleyPlugin* plugin);
// extern "C" DLLEXPORT int getHalleyPluginVersion();
