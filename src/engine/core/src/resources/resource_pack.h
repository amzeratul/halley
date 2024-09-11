#pragma once

#include "halley/resources/resource_locator.h"
#include "halley/resources/asset_database.h"
#include "halley/utils/encrypt.h"

namespace Halley {
	class SystemAPI;
	class AssetPack;

	class PackResourceLocator final : public IResourceLocatorProvider {
	public:
		explicit PackResourceLocator(std::unique_ptr<ResourceDataReader> reader, Path path, std::optional<Encrypt::AESKey> encryptionKey = std::nullopt, bool preLoad = false, std::optional<int> priority = {});
		~PackResourceLocator();

	protected:
		std::unique_ptr<ResourceData> getData(const String& asset, AssetType type, bool stream) override;
		const AssetDatabase& getAssetDatabase() override;
		void purgeAll(SystemAPI& system) override;
		bool purgeIfAffected(SystemAPI& system, gsl::span<const String> assetIds, gsl::span<const String> packIds) override;
		int getPriority() const override;
		size_t getMemoryUsage() const override;
		String getName() const override;
		
	private:
		void loadAfterPurge();

		std::unique_ptr<AssetPack> assetPack;

		Path path;
		bool wasEncrypted = false;
		bool preLoad = false;
		std::optional<int> priority;
		SystemAPI* system = nullptr;
	};
}
