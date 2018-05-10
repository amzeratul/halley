#pragma once

#include "resources/resource_locator.h"
#include "resources/asset_database.h"

namespace Halley {
	class SystemAPI;
	class AssetPack;

	class PackResourceLocator : public IResourceLocatorProvider {
	public:
		explicit PackResourceLocator(std::unique_ptr<ResourceDataReader> reader, const String& encryptionKey = "", bool preLoad = false);
		~PackResourceLocator();

	protected:
		std::unique_ptr<ResourceData> getData(const String& asset, AssetType type, bool stream) override;
		const AssetDatabase& getAssetDatabase() const override;

		std::unique_ptr<AssetPack> assetPack;
	};
}
