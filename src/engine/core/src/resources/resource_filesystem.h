#pragma once

#include "halley/resources/resource_locator.h"
#include "halley/resources/asset_database.h"

namespace Halley {
	class SystemAPI;

	class FileSystemResourceLocator final : public IResourceLocatorProvider {
	public:
		FileSystemResourceLocator(SystemAPI& system, const Path& basePath);

	protected:
		std::unique_ptr<ResourceData> getData(const String& asset, AssetType type, bool stream) override;
		const AssetDatabase& getAssetDatabase() override;
		int getPriority() const override;
		void purgeAll(SystemAPI& system) override;
		bool purgeIfAffected(SystemAPI& system, gsl::span<const String> assetIds, gsl::span<const String> packIds) override;
		String getName() const override;
		size_t getMemoryUsage() const override;

		SystemAPI& system;
		Path basePath;
		std::unique_ptr<AssetDatabase> assetDb;

	private:
		void loadAssetDb();
	};
}
