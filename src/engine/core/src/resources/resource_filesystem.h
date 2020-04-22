#pragma once

#include "resources/resource_locator.h"
#include "resources/asset_database.h"

namespace Halley {
	class SystemAPI;

	class FileSystemResourceLocator final : public IResourceLocatorProvider {
	public:
		FileSystemResourceLocator(SystemAPI& system, const Path& basePath);

	protected:
		std::unique_ptr<ResourceData> getData(const String& asset, AssetType type, bool stream) override;
		const AssetDatabase& getAssetDatabase() override;
		int getPriority() const override;
		void purge(SystemAPI& system) override;

		SystemAPI& system;
		Path basePath;
		std::unique_ptr<AssetDatabase> assetDb;

	private:
		void loadAssetDb();
	};
}
